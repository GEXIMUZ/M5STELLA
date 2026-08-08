#include "w33z_link.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <M5Cardputer.h>
#include <SD.h>
#include <SPIFFS.h>
#include <WiFi.h>

#include "identity.h"
#include "../core/config.h"
#include "../core/network_recon.h"
#include "../gps/gps.h"

namespace StellaLink {
namespace {

struct LinkConfig {
    bool enabled = false;
    bool wifiAutoConnect = false;
    String baseUrl;
    uint32_t heartbeatMs = 5000;
    uint32_t commandPollMs = 1500;
    uint32_t reconnectMs = 10000;
};

LinkConfig cfg;
CommandHandler commandHandler = nullptr;
String id;
String errorText;
String state = "idle";
bool registered = false;
uint32_t lastHeartbeat = 0;
uint32_t lastCommandPoll = 0;
uint32_t lastReconnectAttempt = 0;
bool forceCycle = false;

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

void loadConfig() {
    JsonDocument doc;
    if (!readConfigDocument(doc)) {
        cfg.enabled = false;
        if (errorText.isEmpty()) errorText = "No /stella_link.json; W33Z link disabled.";
        return;
    }

    cfg.enabled = doc["enabled"] | false;
    cfg.wifiAutoConnect = doc["wifiAutoConnect"] | false;
    cfg.baseUrl = trimSlash(String((const char*)(doc["baseUrl"] | "")));
    cfg.heartbeatMs = constrain((uint32_t)(doc["heartbeatMs"] | 5000), 2000UL, 60000UL);
    cfg.commandPollMs = constrain((uint32_t)(doc["commandPollMs"] | 1500), 500UL, 30000UL);
    cfg.reconnectMs = constrain((uint32_t)(doc["reconnectMs"] | 10000), 3000UL, 120000UL);

    if (cfg.enabled && cfg.baseUrl.isEmpty()) {
        cfg.enabled = false;
        errorText = "W33Z link enabled but baseUrl is empty.";
    } else if (cfg.enabled && !cfg.baseUrl.startsWith("http://")) {
        // Local W33Z deployments currently use plain HTTP on the LAN. Keeping
        // TLS out of this first transport also avoids large ESP32 heap spikes.
        cfg.enabled = false;
        errorText = "Stella protocol v1 Wi-Fi transport currently requires http:// baseUrl.";
    }
}

bool ensureWifi() {
    if (WiFi.status() == WL_CONNECTED) return true;
    if (!cfg.wifiAutoConnect) return false;

    const WiFiConfig& wifi = Config::wifi();
    if (wifi.otaSSID[0] == '\0') {
        errorText = "wifiAutoConnect requested but no Wi-Fi SSID is configured.";
        return false;
    }

    uint32_t now = millis();
    if (now - lastReconnectAttempt < cfg.reconnectMs) return false;
    lastReconnectAttempt = now;

    Serial.printf("[STELLA] Connecting to Wi-Fi '%s' for W33Z link...\n", wifi.otaSSID);
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.begin(wifi.otaSSID, wifi.otaPassword);
    return false;
}

void addTelemetry(JsonObject telemetry) {
    telemetry["connected"] = true;
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

bool postJson(const String& path, const String& payload, String* response = nullptr) {
    if (WiFi.status() != WL_CONNECTED) return false;

    HTTPClient http;
    http.setConnectTimeout(2500);
    http.setTimeout(4000);
    const String url = cfg.baseUrl + path;
    if (!http.begin(url)) {
        errorText = "HTTP begin failed: " + url;
        return false;
    }
    http.addHeader("Content-Type", "application/json");
    http.addHeader("User-Agent", "M5STELLA/" + String(StellaIdentity::firmwareVersion()));

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
    if (WiFi.status() != WL_CONNECTED) return false;

    HTTPClient http;
    http.setConnectTimeout(2500);
    http.setTimeout(4000);
    const String url = cfg.baseUrl + path;
    if (!http.begin(url)) {
        errorText = "HTTP begin failed: " + url;
        return false;
    }
    http.addHeader("User-Agent", "M5STELLA/" + String(StellaIdentity::firmwareVersion()));

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
    for (size_t i = 0; i < StellaIdentity::kCapabilityCount; ++i) {
        capabilities.add(StellaIdentity::kCapabilities[i]);
    }

    JsonObject telemetry = handshake["telemetry"].to<JsonObject>();
    addTelemetry(telemetry);

    String payload;
    serializeJson(doc, payload);
    if (!postJson("/api/stella/handshake", payload)) return false;

    registered = true;
    Serial.printf("[STELLA] On leash: %s (%s)\n", id.c_str(), cfg.baseUrl.c_str());
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
    loadConfig();

    Serial.printf("[STELLA] Identity: %s / fw %s / protocol v%u\n",
                  id.c_str(), StellaIdentity::firmwareVersion(), StellaIdentity::kProtocolVersion);
    if (!cfg.enabled) {
        Serial.printf("[STELLA] W33Z link disabled: %s\n", errorText.c_str());
        return;
    }
    Serial.printf("[STELLA] W33Z link enabled -> %s\n", cfg.baseUrl.c_str());
    forceCycle = true;
}

void update() {
    if (!cfg.enabled) return;
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
const String& baseUrl() { return cfg.baseUrl; }
const String& deviceId() { return id; }
const String& lastError() { return errorText; }

void setTechnicalState(const char* nextState) {
    if (!nextState || !*nextState) return;
    state = nextState;
}

void nudge() { forceCycle = true; }

} // namespace StellaLink
