// Configuration management
#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <SPI.h>

// Legacy paths are intentionally retained during the staged migration so
// existing M5PORKCHOP SD cards and saved settings continue to boot.
#define CONFIG_FILE "/porkchop.conf"
#define PERSONALITY_FILE "/personality.json"

// GPS module source selection
enum class GPSSource : uint8_t {
    GROVE = 0,
    CAP_LORA = 1,
    CUSTOM = 2
};

static constexpr uint8_t GPS_SOURCE_COUNT = 3;

namespace CapLoraPins {
    static constexpr uint8_t LORA_CS    = 5;
    static constexpr uint8_t LORA_RESET = 3;
    static constexpr uint8_t LORA_DIO1  = 4;
    static constexpr uint8_t LORA_BUSY  = 6;
    static constexpr uint8_t GPS_RX     = 15;
    static constexpr uint8_t GPS_TX     = 13;
}

struct GPSConfig {
    bool enabled = true;
    GPSSource source = GPSSource::GROVE;
    uint8_t rxPin = 1;
    uint8_t txPin = 2;
    uint32_t baudRate = 115200;
    uint16_t updateInterval = 5;
    uint16_t sleepTimeMs = 5000;
    bool powerSave = true;
    int8_t timezoneOffset = 0;
};

enum class MLCollectionMode : uint8_t {
    BASIC = 0,
    ENHANCED = 1
};

// Internal enum names remain backward-compatible until the UI/mode migration
// is complete. User-facing Stella names are layered above these identifiers.
enum class G0Action : uint8_t {
    SCREEN_TOGGLE = 0,
    OINK,
    DNOHAM,
    SPECTRUM,
    PIGSYNC,
    IDLE
};

static constexpr uint8_t G0_ACTION_COUNT = 6;

enum class BootMode : uint8_t {
    IDLE = 0,
    OINK,
    DNOHAM,
    WARHOG
};

static constexpr uint8_t BOOT_MODE_COUNT = 4;

struct MLConfig {
    bool enabled = true;
    MLCollectionMode collectionMode = MLCollectionMode::ENHANCED;
    // Legacy model path remains readable for existing cards; later migration
    // will introduce /m5stella with an automatic fallback.
    char modelPath[64] = "/m5porkchop/models/porkchop_model.bin";
    float confidenceThreshold = 0.7f;
    float rogueApThreshold = 0.8f;
    float vulnScorerThreshold = 0.6f;
    bool autoUpdate = false;
    char updateUrl[128] = "";
};

struct WiFiConfig {
    uint16_t channelHopInterval = 150;
    uint16_t spectrumHopInterval = 150;
    uint16_t lockTime = 12000;
    bool enableDeauth = true;
    bool randomizeMAC = true;
    int8_t spectrumMinRssi = -95;
    int8_t attackMinRssi = -70;
    uint8_t spectrumTopN = 0;
    uint16_t spectrumStaleMs = 5000;
    bool spectrumCollapseSsid = false;
    bool spectrumTiltEnabled = true;
    char otaSSID[33];
    char otaPassword[65];
    bool autoConnect = false;
    char wpaSecKey[33];
    char wigleApiName[65];
    char wigleApiToken[65];
};

struct BLEConfig {
    uint16_t burstInterval = 200;
    uint16_t advDuration = 100;
};

struct PersonalityConfig {
    char name[32] = "Stella";
    char callsign[16] = "";
    int mood = 50;
    uint32_t experience = 0;
    float curiosity = 0.7f;
    float aggression = 0.3f;
    float patience = 0.5f;
    bool soundEnabled = true;
    uint8_t brightness = 80;
    uint8_t dimLevel = 20;
    uint16_t dimTimeout = 30;
    uint8_t themeIndex = 0;
    G0Action g0Action = G0Action::SCREEN_TOGGLE;
    BootMode bootMode = BootMode::IDLE;
};

class Config {
public:
    static bool init();
    static bool save();
    static bool load();
    static bool loadPersonality();
    static bool isSDAvailable();
    static bool reinitSD();
    static bool loadWpaSecKeyFromFile();
    static bool loadWigleKeyFromFile();
    static void prepareSDBus();
    static void prepareCapLoraGpio();
    static SPIClass& sdSpi();
    static int sdCsPin();

    static GPSConfig& gps() { return gpsConfig; }
    static MLConfig& ml() { return mlConfig; }
    static WiFiConfig& wifi() { return wifiConfig; }
    static BLEConfig& ble() { return bleConfig; }
    static PersonalityConfig& personality() { return personalityConfig; }

    static void setGPS(const GPSConfig& cfg);
    static void setML(const MLConfig& cfg);
    static void setWiFi(const WiFiConfig& cfg);
    static void setBLE(const BLEConfig& cfg);
    static void setPersonality(const PersonalityConfig& cfg);

private:
    static GPSConfig gpsConfig;
    static MLConfig mlConfig;
    static WiFiConfig wifiConfig;
    static BLEConfig bleConfig;
    static PersonalityConfig personalityConfig;
    static bool initialized;

    static bool createDefaultConfig();
    static bool createDefaultPersonality();
    static void savePersonalityToSPIFFS();
    static bool loadFrom(fs::FS& fs, const char* path);
    static bool applyJson(const JsonDocument& doc);
    static bool importCredsFromJsonConf();
};
