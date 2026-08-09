#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected exactly 1 match, found {count}")
    return text.replace(old, new, 1)


# ---------------------------------------------------------------------------
# StellaLink public API
# ---------------------------------------------------------------------------
h = ROOT / "src/stella/w33z_link.h"
text = h.read_text()
text = replace_once(
    text,
    "void nudge();\n\n} // namespace StellaLink\n",
    '''void nudge();

// USB is the trusted bootstrap transport. The browser Device Console requests
// Stella identity over USB Serial, receives a device token from W33Z, then
// provisions that token back over the same cable. No code entry is required.
void showSyncStatus();
bool isPaired();

} // namespace StellaLink
''',
    "w33z_link.h USB API",
)
h.write_text(text)


# ---------------------------------------------------------------------------
# StellaLink implementation
# ---------------------------------------------------------------------------
cpp = ROOT / "src/stella/w33z_link.cpp"
text = cpp.read_text()

text = replace_once(
    text,
    '#include "../gps/gps.h"\n',
    '#include "../gps/gps.h"\n#include "../ui/display.h"\n',
    "w33z_link.cpp Display include",
)

text = replace_once(
    text,
    'Preferences prefs;\n',
    '''Preferences prefs;
String usbRxLine;
static constexpr const char* kDefaultW33zBaseUrl = "https://w33z.gexz.be";
''',
    "w33z_link.cpp USB state",
)

old_load_token = '''void loadStoredToken() {
    if (!prefs.begin("stella-link", false)) return;
    bearerToken = prefs.getString("token", "");
    prefs.end();
}
'''
new_load_token = '''void loadStoredToken() {
    if (!prefs.begin("stella-link", false)) return;
    bearerToken = prefs.getString("token", "");
    const String storedBaseUrl = prefs.getString("base-url", "");
    prefs.end();
    if (!storedBaseUrl.isEmpty()) cfg.baseUrl = trimSlash(storedBaseUrl);
}
'''
text = replace_once(text, old_load_token, new_load_token, "w33z_link.cpp load stored provisioning")

old_save = '''void saveStoredToken(const String& token) {
    if (token.isEmpty()) return;
    if (!prefs.begin("stella-link", false)) return;
    prefs.putString("token", token);
    prefs.end();
    bearerToken = token;
}
'''
new_save = '''void saveStoredToken(const String& token) {
    if (token.isEmpty()) return;
    if (!prefs.begin("stella-link", false)) return;
    prefs.putString("token", token);
    prefs.end();
    bearerToken = token;
}

bool saveUsbProvisioning(const String& token, const String& url) {
    if (token.length() < 32) {
        errorText = "USB enrollment token is invalid.";
        return false;
    }
    String cleanUrl = trimSlash(url);
    if (!cleanUrl.startsWith("https://") && !cleanUrl.startsWith("http://")) {
        errorText = "USB enrollment URL is invalid.";
        return false;
    }
    if (!prefs.begin("stella-link", false)) {
        errorText = "Could not open Stella NVS.";
        return false;
    }
    prefs.putString("token", token);
    prefs.putString("base-url", cleanUrl);
    prefs.end();

    bearerToken = token;
    cfg.baseUrl = cleanUrl;
    cfg.enabled = true;
    cfg.wifiAutoConnect = true;
    cfg.tlsInsecure = true;
    cfg.pairingCode = "";
    registered = false;
    forceCycle = true;
    lastHeartbeat = 0;
    errorText = "";
    return true;
}
'''
text = replace_once(text, old_save, new_save, "w33z_link.cpp USB provisioning save")

old_load_cfg = '''void loadConfig() {
    JsonDocument doc;
    if (!readConfigDocument(doc)) {
        cfg.enabled = false;
        if (errorText.isEmpty()) errorText = "No /stella_link.json; W33Z link disabled.";
        return;
    }

    cfg.enabled = doc["enabled"] | false;
'''
new_load_cfg = '''void loadConfig() {
    JsonDocument doc;
    if (!readConfigDocument(doc)) {
        // First-party default: pairing is bootstrapped over USB and stored in
        // NVS. No SD-card config file is required for normal W33Z use.
        cfg.enabled = true;
        cfg.wifiAutoConnect = true;
        cfg.tlsInsecure = true;
        if (cfg.baseUrl.isEmpty()) cfg.baseUrl = kDefaultW33zBaseUrl;
        cfg.pairingCode = "";
        cfg.heartbeatMs = 5000;
        cfg.commandPollMs = 1500;
        cfg.reconnectMs = 10000;
        errorText = "";
        return;
    }

    cfg.enabled = doc["enabled"] | true;
'''
text = replace_once(text, old_load_cfg, new_load_cfg, "w33z_link.cpp default W33Z config")

text = replace_once(
    text,
    '    cfg.baseUrl = trimSlash(String((const char*)(doc["baseUrl"] | "")));\n',
    '    cfg.baseUrl = trimSlash(String((const char*)(doc["baseUrl"] | kDefaultW33zBaseUrl)));\n',
    "w33z_link.cpp default base URL",
)

# Insert USB protocol helpers immediately before sendHandshake().
needle = '''bool sendHandshake() {
'''
usb_helpers = r'''void emitUsbHello() {
    JsonDocument doc;
    doc["type"] = "stella.hello";
    doc["protocolVersion"] = StellaIdentity::kProtocolVersion;

    JsonObject identity = doc["identity"].to<JsonObject>();
    identity["deviceId"] = id;
    identity["name"] = StellaIdentity::kDeviceName;
    identity["model"] = StellaIdentity::kModel;
    identity["hardwareRevision"] = StellaIdentity::kHardwareRevision;
    identity["firmwareVersion"] = StellaIdentity::firmwareVersion();
    identity["protocolVersion"] = StellaIdentity::kProtocolVersion;
    JsonArray capabilities = identity["capabilities"].to<JsonArray>();
    for (size_t i = 0; i < StellaIdentity::kCapabilityCount; ++i) {
        capabilities.add(StellaIdentity::kCapabilities[i]);
    }

    JsonObject telemetry = doc["telemetry"].to<JsonObject>();
    addTelemetry(telemetry);
    telemetry["connected"] = WiFi.status() == WL_CONNECTED;

    serializeJson(doc, Serial);
    Serial.println();
}

void emitUsbEnrollResult(bool ok, const String& message) {
    JsonDocument doc;
    doc["type"] = "stella.enrolled";
    doc["success"] = ok;
    doc["deviceId"] = id;
    if (!ok) doc["error"] = message;
    serializeJson(doc, Serial);
    Serial.println();
}

void processUsbCommand(const String& line) {
    JsonDocument doc;
    if (deserializeJson(doc, line)) return;
    const String type = String((const char*)(doc["type"] | ""));

    if (type == "hello") {
        emitUsbHello();
        return;
    }

    if (type == "enroll") {
        const String token = String((const char*)(doc["token"] | ""));
        const String url = String((const char*)(doc["baseUrl"] | kDefaultW33zBaseUrl));
        const bool ok = saveUsbProvisioning(token, url);
        emitUsbEnrollResult(ok, ok ? String() : errorText);
        if (ok) {
            Display::setTopBarMessage("W33Z: USB PAIRED", 3500);
            Display::showToast("STELLA LINKED TO W33Z\nWIFI WILL TAKE OVER", 2500);
        }
    }
}

void serviceUsbBootstrap() {
    while (Serial.available() > 0) {
        const char c = (char)Serial.read();
        if (c == '\r') continue;
        if (c == '\n') {
            usbRxLine.trim();
            if (!usbRxLine.isEmpty()) processUsbCommand(usbRxLine);
            usbRxLine = "";
            continue;
        }
        if (usbRxLine.length() < 1536) usbRxLine += c;
        else usbRxLine = "";
    }
}

bool sendHandshake() {
'''
text = replace_once(text, needle, usb_helpers, "w33z_link.cpp USB protocol")

# USB must always be serviced, even before Wi-Fi enrollment exists.
text = replace_once(
    text,
    '''void update() {
    if (!cfg.enabled) return;
''',
    '''void update() {
    serviceUsbBootstrap();
    if (!cfg.enabled) return;
''',
    "w33z_link.cpp USB service update",
)

# Add status API at end.
text = replace_once(
    text,
    '''void nudge() { forceCycle = true; }

} // namespace StellaLink
''',
    '''void nudge() { forceCycle = true; }

bool isPaired() { return !bearerToken.isEmpty(); }

void showSyncStatus() {
    String top;
    String body = "W33Z SYNC\\n";
    if (registered && WiFi.status() == WL_CONNECTED) {
        top = "W33Z: LINKED";
        body += "STATUS: LINKED\\n";
        body += "WIFI: CONNECTED\\n";
        body += id;
    } else if (!bearerToken.isEmpty()) {
        top = WiFi.status() == WL_CONNECTED ? "W33Z: CONNECTING" : "W33Z: WAITING WIFI";
        body += "STATUS: PAIRED\\n";
        body += WiFi.status() == WL_CONNECTED ? "SERVER: CONNECTING\\n" : "WIFI: WAITING\\n";
        body += id;
    } else {
        top = "W33Z: USB PAIR REQUIRED";
        body += "STATUS: UNPAIRED\\n";
        body += "CONNECT USB TO W33Z\\n";
        body += "DEVICE CONSOLE -> CONNECT STELLA";
    }
    Display::setTopBarMessage(top.c_str(), 0);
    Display::showToast(body.c_str(), 4000);
}

} // namespace StellaLink
''',
    "w33z_link.cpp status API",
)

cpp.write_text(text)


# ---------------------------------------------------------------------------
# COMMS -> W33Z SYNC: status shell only. No old ESP-NOW PigSync discovery and
# no Cardputer pairing-code input.
# ---------------------------------------------------------------------------
p = ROOT / "src/core/porkchop.cpp"
text = p.read_text()

text = replace_once(
    text,
    '#include "../modes/charging.h"\n',
    '#include "../modes/charging.h"\n#include "../stella/w33z_link.h"\n',
    "porkchop.cpp StellaLink include",
)

text = replace_once(
    text,
    '''        case PorkchopMode::PIGSYNC_DEVICE_SELECT:
            PigSyncMode::stopDiscovery();
            PigSyncMode::stop();
            break;
''',
    '''        case PorkchopMode::PIGSYNC_DEVICE_SELECT:
            // Legacy enum slot now hosts the W33Z status screen only.
            break;
''',
    "porkchop.cpp W33Z cleanup",
)

text = replace_once(
    text,
    '''        case PorkchopMode::PIGSYNC_DEVICE_SELECT:
            Avatar::setState(AvatarState::EXCITED);
            SDLog::log("PORK", "Mode: PIGSYNC Device Select");
            PigSyncMode::start();
            PigSyncMode::startDiscovery();
            break;
''',
    '''        case PorkchopMode::PIGSYNC_DEVICE_SELECT:
            Avatar::setState(AvatarState::HAPPY);
            SDLog::log("STELLA", "Mode: W33Z SYNC");
            StellaLink::showSyncStatus();
            break;
''',
    "porkchop.cpp W33Z enter",
)

old_input = '''    // In PIGSYNC_DEVICE_SELECT mode, handle navigation and channel switching
    if (currentMode == PorkchopMode::PIGSYNC_DEVICE_SELECT) {
        uint8_t deviceCount = PigSyncMode::getDeviceCount();

        // Handle device navigation (up/down) - only if devices exist
        if (deviceCount > 0) {
            if (M5Cardputer.Keyboard.isKeyPressed(';')) {
                // Up arrow - select previous device
                PigSyncMode::selectDevice(PigSyncMode::getSelectedIndex() > 0 ?
                    PigSyncMode::getSelectedIndex() - 1 : deviceCount - 1);
            }
            if (M5Cardputer.Keyboard.isKeyPressed('.')) {
                // Down arrow - select next device
                PigSyncMode::selectDevice((PigSyncMode::getSelectedIndex() + 1) % deviceCount);
            }
        }

        // Enter to connect to selected device
        if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER) && PigSyncMode::getDeviceCount() > 0) {
            uint8_t selectedIdx = PigSyncMode::getSelectedIndex();
            if (selectedIdx < PigSyncMode::getDeviceCount()) {
                PigSyncMode::connectTo(selectedIdx);
            }
        }

        // A to abort sync (when connected)
        if (PigSyncMode::isConnected() && M5Cardputer.Keyboard.isKeyPressed('a')) {
            if (PigSyncMode::isSyncing()) {
                PigSyncMode::abortSync();
            }
        }

        // D to disconnect (when connected)
        if (PigSyncMode::isConnected() && M5Cardputer.Keyboard.isKeyPressed('d')) {
            PigSyncMode::disconnect();
        }

        // R to rescan (when not connected)
        if (!PigSyncMode::isConnected() && M5Cardputer.Keyboard.isKeyPressed('r')) {
            PigSyncMode::startScan();
        }

        return; // Consume input for PIGSYNC_DEVICE_SELECT
    }
'''
new_input = '''    // W33Z Sync is informational on-device. Pairing itself is done through
    // the trusted USB bootstrap in the W33Z Device Console.
    if (currentMode == PorkchopMode::PIGSYNC_DEVICE_SELECT) {
        if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER) ||
            M5Cardputer.Keyboard.isKeyPressed('r') ||
            M5Cardputer.Keyboard.isKeyPressed('R')) {
            StellaLink::nudge();
            StellaLink::showSyncStatus();
        }
        return;
    }
'''
text = replace_once(text, old_input, new_input, "porkchop.cpp W33Z input")

text = replace_once(
    text,
    '''        case PorkchopMode::PIGSYNC_DEVICE_SELECT:
            // Update PigSync discovery process (includes dialogue phases)
            PigSyncMode::update();
            // Stay in device select mode for terminal display
            if (!PigSyncMode::isRunning()) {
                // User exited, go back to menu
                setMode(PorkchopMode::MENU);
            }
            break;
''',
    '''        case PorkchopMode::PIGSYNC_DEVICE_SELECT:
            // StellaLink::update() runs globally from main.cpp. Nothing heavy
            // belongs in this menu/status shell.
            break;
''',
    "porkchop.cpp W33Z update",
)

p.write_text(text)

print("Applied USB-first W33Z pairing.")
print("Changed:")
print("  src/stella/w33z_link.h")
print("  src/stella/w33z_link.cpp")
print("  src/core/porkchop.cpp")
print("Pairing UX: plug Stella into USB -> W33Z Device Console -> Connect Stella via USB")
print("Next: pio run -e m5cardputer")
