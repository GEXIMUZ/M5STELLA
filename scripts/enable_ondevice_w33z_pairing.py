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
needle = "void nudge();\n\n} // namespace StellaLink\n"
replacement = '''void nudge();

// On-device W33Z enrollment wizard. This is used by COMMS -> W33Z SYNC.
// A successful enrollment stores the bearer token in NVS; the 6-digit code
// itself is intentionally not persisted.
void startPairingWizard();
void handlePairingInput();
void updatePairingWizard();
bool pairingWizardActive();
const String& pairingCodePreview();

} // namespace StellaLink
'''
text = replace_once(text, needle, replacement, "w33z_link.h API")
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
    "w33z_link.cpp display include",
)

text = replace_once(
    text,
    'Preferences prefs;\n',
    '''Preferences prefs;

// Interactive pairing state for COMMS -> W33Z SYNC.
bool pairingUiActive = false;
bool pairingSubmitted = false;
String pairingDigits;
uint32_t pairingLastStatusMs = 0;
static constexpr const char* kDefaultW33zBaseUrl = "https://w33z.gexz.be";
''',
    "w33z_link.cpp pairing state",
)

text = replace_once(
    text,
    '''void saveStoredToken(const String& token) {
    if (token.isEmpty()) return;
    if (!prefs.begin("stella-link", false)) return;
    prefs.putString("token", token);
    prefs.end();
    bearerToken = token;
}
''',
    '''void saveStoredToken(const String& token) {
    if (token.isEmpty()) return;
    if (!prefs.begin("stella-link", false)) return;
    prefs.putString("token", token);
    prefs.end();
    bearerToken = token;
}

void clearStoredToken() {
    if (prefs.begin("stella-link", false)) {
        prefs.remove("token");
        prefs.end();
    }
    bearerToken = "";
}
''',
    "w33z_link.cpp clear token",
)

old_load = '''void loadConfig() {
    JsonDocument doc;
    if (!readConfigDocument(doc)) {
        cfg.enabled = false;
        if (errorText.isEmpty()) errorText = "No /stella_link.json; W33Z link disabled.";
        return;
    }

    cfg.enabled = doc["enabled"] | false;
'''
new_load = '''void loadConfig() {
    JsonDocument doc;
    if (!readConfigDocument(doc)) {
        // Stella is a W33Z-native device. The normal pairing path must work
        // without requiring an SD-card JSON file first. Advanced deployments
        // can still override these values through stella_link.json.
        cfg.enabled = true;
        cfg.wifiAutoConnect = true;
        cfg.tlsInsecure = true;
        cfg.baseUrl = kDefaultW33zBaseUrl;
        cfg.pairingCode = "";
        cfg.heartbeatMs = 5000;
        cfg.commandPollMs = 1500;
        cfg.reconnectMs = 10000;
        errorText = "";
        return;
    }

    cfg.enabled = doc["enabled"] | true;
'''
text = replace_once(text, old_load, new_load, "w33z_link.cpp default config")

# If a config exists but omits baseUrl, keep the first-party endpoint as default.
text = replace_once(
    text,
    '    cfg.baseUrl = trimSlash(String((const char*)(doc["baseUrl"] | "")));\n',
    '    cfg.baseUrl = trimSlash(String((const char*)(doc["baseUrl"] | kDefaultW33zBaseUrl)));\n',
    "w33z_link.cpp default baseUrl",
)

# Append pairing wizard implementation before namespace close.
needle = '''void nudge() { forceCycle = true; }

} // namespace StellaLink
'''
replacement = r'''void nudge() { forceCycle = true; }

void startPairingWizard() {
    pairingUiActive = true;
    pairingSubmitted = false;
    pairingDigits = "";
    pairingLastStatusMs = 0;

    // Make sure the first-party defaults are usable even if an old config
    // disabled the integration.
    cfg.enabled = true;
    if (cfg.baseUrl.isEmpty()) cfg.baseUrl = kDefaultW33zBaseUrl;

    if (!bearerToken.isEmpty()) {
        Display::showToast(
            "W33Z SYNC\nALREADY PAIRED\nTYPE NEW 6-DIGIT CODE TO RE-PAIR",
            3000
        );
    } else {
        Display::showToast(
            "W33Z SYNC\nTYPE 6-DIGIT PAIRING CODE\nENTER = PAIR   ESC = EXIT",
            3000
        );
    }
    Display::setTopBarMessage("W33Z CODE: ______", 0);
}

void handlePairingInput() {
    if (!pairingUiActive || !M5Cardputer.Keyboard.isChange()) return;

    // Once a request was submitted, Backspace lets the user immediately retry
    // with a fresh code instead of leaving/re-entering the menu.
    if (pairingSubmitted && M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE)) {
        pairingSubmitted = false;
        pairingDigits = "";
        errorText = "";
        Display::setTopBarMessage("W33Z CODE: ______", 0);
        return;
    }

    if (!pairingSubmitted && M5Cardputer.Keyboard.isKeyPressed(KEY_BACKSPACE)) {
        if (!pairingDigits.isEmpty()) pairingDigits.remove(pairingDigits.length() - 1);
    }

    if (!pairingSubmitted) {
        auto keys = M5Cardputer.Keyboard.keysState();
        for (char c : keys.word) {
            if (c >= '0' && c <= '9' && pairingDigits.length() < 6) {
                pairingDigits += c;
            }
        }

        String preview = "W33Z CODE: ";
        preview += pairingDigits;
        for (size_t i = pairingDigits.length(); i < 6; ++i) preview += '_';
        Display::setTopBarMessage(preview.c_str(), 0);

        if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
            if (pairingDigits.length() != 6) {
                Display::showToast("PAIRING CODE MUST BE 6 DIGITS", 1800);
                return;
            }

            // A new code explicitly means re-enrollment. Drop the old bearer
            // token so sendHandshake() sends the short-lived pairing code.
            clearStoredToken();
            registered = false;
            cfg.pairingCode = pairingDigits;
            cfg.enabled = true;
            pairingSubmitted = true;
            errorText = "";
            forceCycle = true;
            lastHeartbeat = 0;
            Display::setTopBarMessage("W33Z: PAIRING...", 0);
            Display::showToast("PAIRING STELLA WITH W33Z...", 1600);
        }
    }
}

void updatePairingWizard() {
    if (!pairingUiActive) return;

    if (registered && !bearerToken.isEmpty()) {
        pairingUiActive = false;
        pairingSubmitted = false;
        cfg.pairingCode = "";
        Display::setTopBarMessage("W33Z LINKED", 3500);
        Display::showToast("STELLA LINKED TO W33Z", 2500);
        return;
    }

    if (!pairingSubmitted) return;

    const uint32_t now = millis();
    if (now - pairingLastStatusMs < 1200) return;
    pairingLastStatusMs = now;

    if (WiFi.status() != WL_CONNECTED) {
        Display::setTopBarMessage("W33Z: WAITING FOR WIFI", 0);
    } else if (!errorText.isEmpty()) {
        String msg = "W33Z: ";
        msg += errorText;
        Display::setTopBarMessage(msg.c_str(), 0);
    } else {
        Display::setTopBarMessage("W33Z: PAIRING...", 0);
    }
}

bool pairingWizardActive() { return pairingUiActive; }
const String& pairingCodePreview() { return pairingDigits; }

} // namespace StellaLink
'''
text = replace_once(text, needle, replacement, "w33z_link.cpp wizard implementation")
cpp.write_text(text)


# ---------------------------------------------------------------------------
# Re-purpose the old PigSync menu shell as W33Z Sync UI.
# The enum name stays internal for now to minimize state-machine churn.
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
            // W33Z Sync uses this legacy enum slot as a lightweight UI shell.
            // No ESP-NOW PigSync discovery is started here anymore.
            break;
''',
    "porkchop.cpp mode cleanup",
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
            Avatar::setState(AvatarState::EXCITED);
            SDLog::log("STELLA", "Mode: W33Z SYNC");
            StellaLink::startPairingWizard();
            break;
''',
    "porkchop.cpp mode enter",
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
new_input = '''    // W33Z Sync pairing wizard. The legacy PIGSYNC enum value is retained
    // internally only so this change does not disturb the rest of the mode ABI.
    if (currentMode == PorkchopMode::PIGSYNC_DEVICE_SELECT) {
        StellaLink::handlePairingInput();
        return;
    }
'''
text = replace_once(text, old_input, new_input, "porkchop.cpp pairing input")

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
            StellaLink::updatePairingWizard();
            // Successful pairing closes the wizard and returns to COMMS/menu.
            if (!StellaLink::pairingWizardActive()) {
                setMode(PorkchopMode::MENU);
            }
            break;
''',
    "porkchop.cpp pairing update",
)

p.write_text(text)

print("Applied on-device W33Z pairing integration.")
print("Changed:")
print("  src/stella/w33z_link.h")
print("  src/stella/w33z_link.cpp")
print("  src/core/porkchop.cpp")
print("Next: pio run -e m5cardputer")
