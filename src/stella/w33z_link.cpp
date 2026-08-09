#include "w33z_link.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <M5Cardputer.h>
#include <Preferences.h>
#include <SD.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <esp_heap_caps.h>

#include "identity.h"
#include "device_console.h"
#include "../core/config.h"
#include "../core/network_recon.h"
#include "../core/porkchop.h"
#include "../gps/gps.h"

extern Porkchop porkchop;

namespace StellaLink {
namespace {

static constexpr const char* kDefaultW33zBaseUrl = "https://w33z.gexz.be";
static constexpr size_t kMaxUsbLineLength = 1536;
static constexpr size_t kMaxPendingResults = 8;

struct LinkConfig {
    bool enabled = true;
    bool wifiAutoConnect = true;
    bool tlsInsecure = true;
    String baseUrl = kDefaultW33zBaseUrl;
    String pairingCode;
    uint32_t heartbeatMs = 5000;
    uint32_t commandPollMs = 3000; // retained for config compatibility; sync now piggybacks commands
    uint32_t reconnectMs = 10000;
};

struct PendingResult {
    bool used = false;
    String commandId;
    bool success = false;
    String result;
    String error;
};

LinkConfig cfg;
CommandHandler commandHandler = nullptr;
String id;
String errorText;
String state = "idle";
String bearerToken;
String usbRxLine;
bool registered = false;
bool reconPausedForW33z = false;
bool controlPlaneWasActive = false;
uint32_t lastHeartbeat = 0;
uint32_t lastReconnectAttempt = 0;
uint32_t lastSuccessfulSync = 0;
uint32_t consecutiveHttpFailures = 0;
int lastHttpStatus = 0;
bool forceCycle = false;
Preferences prefs;
PendingResult pendingResults[kMaxPendingResults];

// Keep transport objects alive across requests. Arduino-ESP32 HTTPClient 2.0.17
// can reuse an HTTP/1.1 connection, but only while the underlying Client and
// HTTPClient objects survive. Creating these on the stack caused a full TLS
// teardown/handshake on every heartbeat and command poll.
WiFiClientSecure secureClient;
WiFiClient plainClient;
HTTPClient httpClient;

String trimSlash(String value) {
    value.trim();
    while (value.endsWith("/")) value.remove(value.length() - 1);
    return value;
}

void resetHttpTransport() {
    httpClient.setReuse(false);
    httpClient.end();
    secureClient.stop();
    plainClient.stop();
    httpClient.setReuse(true);
}

bool isControlPlaneMode() {
    return porkchop.getMode() == PorkchopMode::PIGSYNC_DEVICE_SELECT;
}

bool maintainControlPlaneRadioLease() {
    const bool active = isControlPlaneMode();

    if (active) {
        // W33Z Sync owns the single ESP32-S3 Wi-Fi radio. Background recon must
        // stay parked for the entire time this screen is active; otherwise its
        // channel hopper tears down the infrastructure Wi-Fi association.
        if (NetworkRecon::isRunning() && !NetworkRecon::isPaused()) {
            NetworkRecon::pause();
            reconPausedForW33z = true;
            registered = false;
            forceCycle = true;
            lastReconnectAttempt = 0;
            resetHttpTransport();
        }

        if (!controlPlaneWasActive) {
            controlPlaneWasActive = true;
            registered = false;
            forceCycle = true;
            lastHeartbeat = 0;
            lastReconnectAttempt = 0;
        }
        return true;
    }

    if (controlPlaneWasActive) {
        controlPlaneWasActive = false;
        registered = false;
        forceCycle = false;
        resetHttpTransport();
    }

    if (reconPausedForW33z) {
        if (NetworkRecon::isPaused()) {
            NetworkRecon::resume();
        }
        reconPausedForW33z = false;
    }

    return false;
}

bool readConfigDocument(JsonDocument& doc) {
    File file;

    if (Config::isSDAvailable()) {
        if (SD.exists("/stella_link.json")) file = SD.open("/stella_link.json", FILE_READ);
        else if (SD.exists("/m5stella/config/stella_link.json")) file = SD.open("/m5stella/config/stella_link.json", FILE_READ);
    }

    if (!file && SPIFFS.begin(false) && SPIFFS.exists("/stella_link.json")) {
        file = SPIFFS.open("/stella_link.json", FILE_READ);
    }

    if (!file) return false;
    DeserializationError err = deserializeJson(doc, file);
    file.close();
    if (err) {
        errorText = String("stella_link.json: ") + err.c_str();
        return false;
    }
    return true;
}

void loadStoredProvisioning() {
    if (!prefs.begin("stella-link", true)) return;
    bearerToken = prefs.getString("token", "");
    const String storedBaseUrl = prefs.getString("base-url", "");
    prefs.end();

    if (!storedBaseUrl.isEmpty()) cfg.baseUrl = trimSlash(storedBaseUrl);
}

void saveStoredToken(const String& token) {
    if (token.isEmpty()) return;
    if (!prefs.begin("stella-link", false)) return;
    prefs.putString("token", token);
    prefs.end();
    bearerToken = token;
}

bool saveUsbProvisioning(const String& token, const String& baseUrl) {
    String cleanToken = token;
    cleanToken.trim();
    String cleanUrl = trimSlash(baseUrl);

    if (cleanToken.length() < 32) {
        errorText = "USB enrollment token is invalid.";
        return false;
    }
    if (!cleanUrl.startsWith("https://") && !cleanUrl.startsWith("http://")) {
        errorText = "USB enrollment URL is invalid.";
        return false;
    }

    if (!prefs.begin("stella-link", false)) {
        errorText = "Could not open Stella NVS.";
        return false;
    }
    prefs.putString("token", cleanToken);
    prefs.putString("base-url", cleanUrl);
    prefs.end();

    bearerToken = cleanToken;
    cfg.baseUrl = cleanUrl;
    cfg.enabled = true;
    cfg.wifiAutoConnect = true;
    cfg.pairingCode = "";
    registered = false;
    forceCycle = true;
    lastHeartbeat = 0;
    lastReconnectAttempt = 0;
    errorText = "";
    resetHttpTransport();
    return true;
}

void loadConfig() {
    JsonDocument doc;
    if (!readConfigDocument(doc)) {
        cfg.enabled = true;
        cfg.wifiAutoConnect = true;
        cfg.tlsInsecure = true;
        if (cfg.baseUrl.isEmpty()) cfg.baseUrl = kDefaultW33zBaseUrl;
        cfg.pairingCode = "";
        cfg.heartbeatMs = 5000;
        cfg.commandPollMs = 3000;
        cfg.reconnectMs = 10000;
        errorText = "";
        return;
    }

    cfg.enabled = doc["enabled"] | true;
    cfg.wifiAutoConnect = doc["wifiAutoConnect"] | true;
    cfg.tlsInsecure = doc["tlsInsecure"] | true;
    cfg.baseUrl = trimSlash(String((const char*)(doc["baseUrl"] | cfg.baseUrl.c_str())));
    cfg.pairingCode = String((const char*)(doc["pairingCode"] | ""));
    cfg.pairingCode.trim();
    cfg.heartbeatMs = constrain((uint32_t)(doc["heartbeatMs"] | 5000), 3000UL, 60000UL);
    cfg.commandPollMs = constrain((uint32_t)(doc["commandPollMs"] | 3000), 1000UL, 30000UL);
    cfg.reconnectMs = constrain((uint32_t)(doc["reconnectMs"] | 10000), 3000UL, 120000UL);

    if (cfg.baseUrl.isEmpty()) cfg.baseUrl = kDefaultW33zBaseUrl;
    if (!cfg.baseUrl.startsWith("http://") && !cfg.baseUrl.startsWith("https://")) {
        cfg.enabled = false;
        errorText = "W33Z baseUrl must start with http:// or https://";
    }
}

bool ensureWifi() {
    if (WiFi.status() == WL_CONNECTED) return true;
    if (!cfg.wifiAutoConnect) return false;

    if (registered) registered = false;
    resetHttpTransport();

    const WiFiConfig& wifi = Config::wifi();
    if (wifi.otaSSID[0] == '\0') {
        errorText = "No Wi-Fi SSID configured for W33Z.";
        return false;
    }

    const uint32_t now = millis();
    if (now - lastReconnectAttempt < cfg.reconnectMs) return false;
    lastReconnectAttempt = now;

    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.begin(wifi.otaSSID, wifi.otaPassword);
    return false;
}

void addTelemetry(JsonObject telemetry) {
    const bool wifiConnected = WiFi.status() == WL_CONNECTED;
    telemetry["connected"] = wifiConnected;
    telemetry["state"] = state;
    telemetry["batteryPercent"] = M5.Power.getBatteryLevel();

    JsonObject radio = telemetry["radio"].to<JsonObject>();
    radio["channel"] = wifiConnected ? WiFi.channel() : NetworkRecon::getCurrentChannel();
    radio["reconChannel"] = NetworkRecon::getCurrentChannel();
    radio["networksSeen"] = NetworkRecon::getNetworkCount();
    radio["reconRunning"] = NetworkRecon::isRunning();
    radio["reconPaused"] = NetworkRecon::isPaused();
    if (wifiConnected) radio["rssi"] = WiFi.RSSI();

    JsonObject link = telemetry["link"].to<JsonObject>();
    link["registered"] = registered;
    link["httpStatus"] = lastHttpStatus;
    link["httpFailures"] = consecutiveHttpFailures;
    link["heapFree"] = ESP.getFreeHeap();
    link["heapLargest"] = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    link["controlPlane"] = controlPlaneWasActive;
    if (lastSuccessfulSync > 0) link["lastSyncAgeMs"] = millis() - lastSuccessfulSync;
    if (!errorText.isEmpty()) link["error"] = errorText;

    if (Config::gps().enabled) {
        GPSData data = GPS::getData();
        JsonObject gps = telemetry["gps"].to<JsonObject>();
        gps["fix"] = data.fix;
        gps["satellites"] = data.satellites;
        if (data.fix) {
            gps["latitude"] = data.latitude;
            gps["longitude"] = data.longitude;
        }
    }
}

void emitUsbHello() {
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
    const DeserializationError err = deserializeJson(doc, line);
    if (err) return;

    if (StellaDeviceConsole::handleUsbCommand(doc)) return;

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
    }
}

void serviceUsbBootstrapInternal() {
    while (Serial.available() > 0) {
        const char c = static_cast<char>(Serial.read());
        if (c == '\r') continue;

        if (c == '\n') {
            usbRxLine.trim();
            if (!usbRxLine.isEmpty()) processUsbCommand(usbRxLine);
            usbRxLine = "";
            continue;
        }

        if (usbRxLine.length() < kMaxUsbLineLength) {
            usbRxLine += c;
        } else {
            usbRxLine = "";
        }
    }
}

bool beginHttp(const String& path) {
    if (WiFi.status() != WL_CONNECTED || bearerToken.isEmpty()) return false;

    const String url = cfg.baseUrl + path;
    httpClient.setReuse(true);
    httpClient.setConnectTimeout(3500);
    httpClient.setTimeout(6000);

    bool ok = false;
    if (url.startsWith("https://")) {
        if (!cfg.tlsInsecure) {
            errorText = "HTTPS currently requires tlsInsecure=true until CA pinning is configured.";
            return false;
        }
        secureClient.setInsecure();
        ok = httpClient.begin(secureClient, url);
    } else {
        ok = httpClient.begin(plainClient, url);
    }

    if (!ok) {
        errorText = "HTTP begin failed: " + url;
        resetHttpTransport();
        return false;
    }

    httpClient.addHeader("User-Agent", "M5STELLA/" + String(StellaIdentity::firmwareVersion()));
    httpClient.addHeader("Authorization", "Bearer " + bearerToken);
    return true;
}

void recordHttpResult(int statusCode, const String& path) {
    lastHttpStatus = statusCode;
    if (statusCode >= 200 && statusCode < 300) {
        consecutiveHttpFailures = 0;
        errorText = "";
        lastSuccessfulSync = millis();
        return;
    }

    consecutiveHttpFailures++;
    errorText = "W33Z " + path + " failed HTTP " + String(statusCode);
    resetHttpTransport();
}

bool postJson(const String& path, const String& payload, String* response = nullptr) {
    if (!beginHttp(path)) return false;

    httpClient.addHeader("Content-Type", "application/json");
    const int statusCode = httpClient.POST(payload);
    if (response) *response = statusCode > 0 ? httpClient.getString() : String();
    httpClient.end();
    recordHttpResult(statusCode, path);
    return statusCode >= 200 && statusCode < 300;
}

bool sendHandshake() {
    if (bearerToken.isEmpty()) return false;

    JsonDocument doc;
    doc["transport"] = "wifi";

    JsonObject handshake = doc["handshake"].to<JsonObject>();
    handshake["protocolVersion"] = StellaIdentity::kProtocolVersion;

    JsonObject identity = handshake["identity"].to<JsonObject>();
    identity["deviceId"] = id;
    identity["name"] = StellaIdentity::kDeviceName;
    identity["model"] = StellaIdentity::kModel;
    identity["hardwareRevision"] = StellaIdentity::kHardwareRevision;
    identity["firmwareVersion"] = StellaIdentity::firmwareVersion();
    identity["protocolVersion"] = StellaIdentity::kProtocolVersion;
    JsonArray capabilities = identity["capabilities"].to<JsonArray>();
    for (size_t i = 0; i < StellaIdentity::kCapabilityCount; ++i) capabilities.add(StellaIdentity::kCapabilities[i]);

    JsonObject telemetry = handshake["telemetry"].to<JsonObject>();
    addTelemetry(telemetry);

    String payload;
    serializeJson(doc, payload);
    String response;
    if (!postJson("/api/stella/handshake", payload, &response)) return false;

    JsonDocument reply;
    if (deserializeJson(reply, response)) {
        errorText = "Invalid W33Z handshake response.";
        return false;
    }
    if (!(reply["success"] | false)) {
        errorText = String((const char*)(reply["error"] | "W33Z rejected handshake"));
        return false;
    }

    const char* issuedToken = reply["token"] | nullptr;
    if (issuedToken && *issuedToken) saveStoredToken(String(issuedToken));

    registered = true;
    return true;
}

CommandAction decodeCommand(const String& command) {
    if (command == "scan.start") return CommandAction::SNIFF;
    if (command == "scan.pause") return CommandAction::STAY;
    if (command == "scan.stop") return CommandAction::HEEL;
    if (command == "sync.pull") return CommandAction::FETCH;
    if (command == "sync.push") return CommandAction::DELIVER;
    if (command == "device.identify") return CommandAction::BARK;
    if (command == "device.reboot") return CommandAction::REBOOT;
    if (command == "display.snapshot") return CommandAction::DISPLAY_SNAPSHOT;
    if (command == "logs.tail") return CommandAction::LOGS_TAIL;
    if (command == "firmware.prepare") return CommandAction::FIRMWARE_PREPARE;
    return CommandAction::NONE;
}

void queueCommandResult(const String& commandId, bool success, const String& result, const String& error) {
    if (commandId.isEmpty()) return;

    size_t slot = kMaxPendingResults;
    for (size_t i = 0; i < kMaxPendingResults; ++i) {
        if (!pendingResults[i].used) {
            slot = i;
            break;
        }
    }
    if (slot == kMaxPendingResults) slot = 0;

    pendingResults[slot].used = true;
    pendingResults[slot].commandId = commandId;
    pendingResults[slot].success = success;
    pendingResults[slot].result = result;
    pendingResults[slot].error = error;
}

void addPendingResults(JsonArray results) {
    for (size_t i = 0; i < kMaxPendingResults; ++i) {
        if (!pendingResults[i].used) continue;
        JsonObject item = results.add<JsonObject>();
        item["id"] = pendingResults[i].commandId;
        item["success"] = pendingResults[i].success;
        if (pendingResults[i].success) item["result"] = pendingResults[i].result;
        else item["error"] = pendingResults[i].error;
    }
}

void clearPendingResults() {
    for (size_t i = 0; i < kMaxPendingResults; ++i) {
        pendingResults[i].used = false;
        pendingResults[i].commandId = "";
        pendingResults[i].result = "";
        pendingResults[i].error = "";
    }
}

void executeCommands(JsonArray commands) {
    for (JsonObject commandObj : commands) {
        const String commandId = String((const char*)(commandObj["id"] | ""));
        const String commandName = String((const char*)(commandObj["command"] | ""));
        const CommandAction action = decodeCommand(commandName);

        String payload;
        if (!commandObj["payload"].isNull()) serializeJson(commandObj["payload"], payload);

        bool ok = false;
        String result;
        String commandError;

        if (action == CommandAction::NONE) {
            commandError = "Unsupported command in Stella firmware: " + commandName;
        } else if (!commandHandler) {
            commandError = "No Stella command handler registered.";
        } else {
            ok = commandHandler(action, payload, result);
            if (!ok && result.length()) commandError = result;
        }

        queueCommandResult(commandId, ok, result, commandError);
    }
}

bool performSyncCycle() {
    if (!registered && !sendHandshake()) return false;

    JsonDocument doc;
    JsonObject telemetry = doc["telemetry"].to<JsonObject>();
    addTelemetry(telemetry);
    JsonArray results = doc["results"].to<JsonArray>();
    addPendingResults(results);

    String payload;
    serializeJson(doc, payload);
    String response;
    if (!postJson("/api/stella/devices/" + id + "/sync", payload, &response)) {
        registered = false;
        return false;
    }

    // A 2xx response means the server accepted all result acknowledgements.
    clearPendingResults();

    JsonDocument reply;
    if (deserializeJson(reply, response) || !(reply["success"] | false)) {
        errorText = "Invalid W33Z sync response.";
        registered = false;
        return false;
    }

    registered = true;
    JsonArray commands = reply["commands"].as<JsonArray>();
    if (!commands.isNull()) executeCommands(commands);
    return true;
}

} // namespace

void init(CommandHandler handler) {
    commandHandler = handler;
    id = StellaIdentity::deviceId();
    loadStoredProvisioning();
    loadConfig();

    httpClient.setReuse(true);
    secureClient.setInsecure();

    forceCycle = !bearerToken.isEmpty();
}

void serviceUsbBootstrap() {
    serviceUsbBootstrapInternal();
}

void update() {
    serviceUsbBootstrapInternal();

    if (!cfg.enabled || bearerToken.isEmpty()) return;

    // The authenticated Wi-Fi control plane is intentionally active only while
    // the on-device W33Z Sync screen owns the radio. USB display/control remains
    // available regardless of Wi-Fi state.
    if (!maintainControlPlaneRadioLease()) return;

    if (!ensureWifi()) return;

    const uint32_t now = millis();
    if (forceCycle || now - lastHeartbeat >= cfg.heartbeatMs) {
        lastHeartbeat = now;
        forceCycle = false;
        performSyncCycle();
    }
}

bool isEnabled() { return cfg.enabled; }
bool isConnected() { return WiFi.status() == WL_CONNECTED; }
bool isRegistered() { return registered; }
bool isPaired() { return !bearerToken.isEmpty(); }
const String& baseUrl() { return cfg.baseUrl; }
const String& deviceId() { return id; }
const String& lastError() { return errorText; }

void setTechnicalState(const char* nextState) {
    if (!nextState || !*nextState) return;
    state = nextState;
}

void nudge() {
    if (!bearerToken.isEmpty()) forceCycle = true;
}

} // namespace StellaLink
