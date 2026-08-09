#include "w33z_link.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <M5Cardputer.h>
#include <Preferences.h>
#include <SD.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "identity.h"
#include "../core/config.h"
#include "../core/network_recon.h"
#include "../gps/gps.h"

namespace StellaLink {
namespace {

static constexpr const char* kDefaultW33zBaseUrl = "https://w33z.gexz.be";
static constexpr size_t kMaxUsbLineLength = 1536;

struct LinkConfig {
    bool enabled = true;
    bool wifiAutoConnect = true;
    bool tlsInsecure = true;
    String baseUrl = kDefaultW33zBaseUrl;
    String pairingCode;
    uint32_t heartbeatMs = 5000;
    uint32_t commandPollMs = 1500;
    uint32_t reconnectMs = 10000;
};

LinkConfig cfg;
CommandHandler commandHandler = nullptr;
String id;
String errorText;
String state = "idle";
String bearerToken;
String usbRxLine;
bool registered = false;
uint32_t lastHeartbeat = 0;
uint32_t lastCommandPoll = 0;
uint32_t lastReconnectAttempt = 0;
bool forceCycle = false;
Preferences prefs;

String trimSlash(String value) {
    value.trim();
    while (value.endsWith("/")) value.remove(value.length() - 1);
    return value;
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

    // Current W33Z tokens are 32 random bytes encoded as 64 hex chars. Keep
    // validation slightly future-proof while still rejecting accidental input.
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
    lastCommandPoll = 0;
    errorText = "";
    return true;
}

void loadConfig() {
    JsonDocument doc;
    if (!readConfigDocument(doc)) {
        // First-party defaults: USB enrollment + NVS is the normal path.
        // No SD-card config file is required.
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
    cfg.wifiAutoConnect = doc["wifiAutoConnect"] | true;
    cfg.tlsInsecure = doc["tlsInsecure"] | true;
    cfg.baseUrl = trimSlash(String((const char*)(doc["baseUrl"] | cfg.baseUrl.c_str())));
    cfg.pairingCode = String((const char*)(doc["pairingCode"] | ""));
    cfg.pairingCode.trim();
    cfg.heartbeatMs = constrain((uint32_t)(doc["heartbeatMs"] | 5000), 2000UL, 60000UL);
    cfg.commandPollMs = constrain((uint32_t)(doc["commandPollMs"] | 1500), 500UL, 30000UL);
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

    const WiFiConfig& wifi = Config::wifi();
    if (wifi.otaSSID[0] == '\0') {
        errorText = "No Wi-Fi SSID configured for W33Z.";
        return false;
    }

    const uint32_t now = millis();
    if (now - lastReconnectAttempt < cfg.reconnectMs) return false;
    lastReconnectAttempt = now;

    Serial.printf("[STELLA] Connecting to Wi-Fi '%s' for W33Z link...\n", wifi.otaSSID);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.begin(wifi.otaSSID, wifi.otaPassword);
    return false;
}

void addTelemetry(JsonObject telemetry) {
    telemetry["connected"] = WiFi.status() == WL_CONNECTED;
    telemetry["state"] = state;
    telemetry["batteryPercent"] = M5.Power.getBatteryLevel();

    JsonObject radio = telemetry["radio"].to<JsonObject>();
    radio["channel"] = NetworkRecon::getCurrentChannel();
    radio["networksSeen"] = NetworkRecon::getNetworkCount();

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
    if (err) return; // Ignore normal firmware log/noise safely.

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
            Serial.printf("[STELLA] USB enrollment stored for %s -> %s\n", id.c_str(), cfg.baseUrl.c_str());
        }
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
            // Drop oversized/malformed input and resynchronize at next newline.
            usbRxLine = "";
        }
    }
}

void addCommonHeaders(HTTPClient& http) {
    http.addHeader("User-Agent", "M5STELLA/" + String(StellaIdentity::firmwareVersion()));
    if (!bearerToken.isEmpty()) {
        http.addHeader("Authorization", "Bearer " + bearerToken);
    }
}

bool beginHttp(HTTPClient& http, WiFiClientSecure& secure, const String& url) {
    if (url.startsWith("https://")) {
        // Phase 1: encrypted transport to W33Z. Certificate pinning/CA bundle can
        // replace setInsecure after the public deployment path is stable.
        if (!cfg.tlsInsecure) {
            errorText = "HTTPS currently requires tlsInsecure=true until CA pinning is configured.";
            return false;
        }
        secure.setInsecure();
        return http.begin(secure, url);
    }
    return http.begin(url);
}

bool postJson(const String& path, const String& payload, String* response = nullptr) {
    if (WiFi.status() != WL_CONNECTED || bearerToken.isEmpty()) return false;

    HTTPClient http;
    WiFiClientSecure secure;
    http.setConnectTimeout(3500);
    http.setTimeout(6000);
    const String url = cfg.baseUrl + path;
    if (!beginHttp(http, secure, url)) {
        if (errorText.isEmpty()) errorText = "HTTP begin failed: " + url;
        return false;
    }
    http.addHeader("Content-Type", "application/json");
    addCommonHeaders(http);

    const int statusCode = http.POST(payload);
    if (response) *response = statusCode > 0 ? http.getString() : String();
    http.end();

    if (statusCode < 200 || statusCode >= 300) {
        errorText = "W33Z POST " + path + " failed HTTP " + String(statusCode);
        return false;
    }
    errorText = "";
    return true;
}

bool getJson(const String& path, String& response) {
    if (WiFi.status() != WL_CONNECTED || bearerToken.isEmpty()) return false;

    HTTPClient http;
    WiFiClientSecure secure;
    http.setConnectTimeout(3500);
    http.setTimeout(6000);
    const String url = cfg.baseUrl + path;
    if (!beginHttp(http, secure, url)) {
        if (errorText.isEmpty()) errorText = "HTTP begin failed: " + url;
        return false;
    }
    addCommonHeaders(http);

    const int statusCode = http.GET();
    response = statusCode > 0 ? http.getString() : String();
    http.end();

    if (statusCode < 200 || statusCode >= 300) {
        errorText = "W33Z GET " + path + " failed HTTP " + String(statusCode);
        return false;
    }
    errorText = "";
    return true;
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

    // Pairing normally happens over USB. Keep support for a future server-side
    // token rotation response without changing the transport contract.
    const char* issuedToken = reply["token"] | nullptr;
    if (issuedToken && *issuedToken) {
        saveStoredToken(String(issuedToken));
        Serial.println("[STELLA] Rotated W33Z token stored in NVS.");
    }

    registered = true;
    Serial.printf("[STELLA] W33Z linked: %s (%s)\n", id.c_str(), cfg.baseUrl.c_str());
    return true;
}

bool sendTelemetry() {
    if (!registered) return sendHandshake();

    JsonDocument doc;
    JsonObject telemetry = doc.to<JsonObject>();
    addTelemetry(telemetry);
    String payload;
    serializeJson(doc, payload);

    if (!postJson("/api/stella/devices/" + id + "/telemetry", payload)) {
        registered = false;
        return false;
    }
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

void reportCommandResult(const String& commandId, bool success, const String& result, const String& error) {
    JsonDocument doc;
    doc["success"] = success;
    if (success) doc["result"] = result;
    else doc["error"] = error;
    String payload;
    serializeJson(doc, payload);
    postJson("/api/stella/devices/" + id + "/commands/" + commandId + "/result", payload);
}

void pollCommands() {
    if (!registered) return;

    String response;
    if (!getJson("/api/stella/devices/" + id + "/commands", response)) {
        registered = false;
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, response);
    if (err || !doc["success"].as<bool>()) return;

    JsonArray commands = doc["commands"].as<JsonArray>();
    for (JsonObject commandObj : commands) {
        String commandId = String((const char*)(commandObj["id"] | ""));
        String commandName = String((const char*)(commandObj["command"] | ""));
        CommandAction action = decodeCommand(commandName);

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

        reportCommandResult(commandId, ok, result, commandError);
    }
}

} // namespace

void init(CommandHandler handler) {
    commandHandler = handler;
    id = StellaIdentity::deviceId();
    loadStoredProvisioning();
    loadConfig();

    Serial.printf("[STELLA] Identity: %s / fw %s / protocol v%u\n",
                  id.c_str(), StellaIdentity::firmwareVersion(), StellaIdentity::kProtocolVersion);
    Serial.printf("[STELLA] W33Z USB bootstrap ready | paired=%s | endpoint=%s\n",
                  bearerToken.isEmpty() ? "no" : "yes", cfg.baseUrl.c_str());

    if (!cfg.enabled) {
        Serial.printf("[STELLA] W33Z Wi-Fi link disabled by config: %s\n", errorText.c_str());
    }

    // Only enrolled devices may enter the Wi-Fi/TLS state machine.
    forceCycle = !bearerToken.isEmpty();
}

void serviceUsbBootstrap() {
    serviceUsbBootstrapInternal();
}

void update() {
    // Keep this call too so callers that do not use main.cpp's priority service
    // still get USB bootstrap handling.
    serviceUsbBootstrapInternal();

    if (!cfg.enabled) return;

    // CRITICAL: USB is the trust bootstrap. Never allocate Wi-Fi/TLS resources
    // merely because Stella booted. This also prevents pre-pair TLS heap spam.
    if (bearerToken.isEmpty()) return;

    if (!ensureWifi()) return;

    const uint32_t now = millis();
    if (forceCycle || now - lastHeartbeat >= cfg.heartbeatMs) {
        lastHeartbeat = now;
        forceCycle = false;
        sendTelemetry();
    }

    if (registered && now - lastCommandPoll >= cfg.commandPollMs) {
        lastCommandPoll = now;
        pollCommands();
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
