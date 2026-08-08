// M5STELLA
// Main entry point
// Based on M5PORKCHOP by 0ct0, evolved under the MIT license.

#include <M5Cardputer.h>
#include <M5Unified.h>
#include <SD.h>
#include <WiFi.h>
#include <esp_heap_caps.h>
#include <string.h>
#include "core/porkchop.h"
#include "core/config.h"
#include "core/xp.h"
#include "core/sdlog.h"
#include "core/wifi_utils.h"
#include "core/heap_policy.h"
#include "core/heap_health.h"
#include "core/network_recon.h"
#include "ui/display.h"
#include "gps/gps.h"
#include "piglet/avatar.h"
#include "piglet/mood.h"
#include "modes/oink.h"
#include "modes/warhog.h"
#include "audio/sfx.h"
#include "stella/identity.h"
#include "stella/w33z_link.h"

// Legacy core controller retained during the staged Stella refactor.
// New integrations should talk through StellaLink rather than coupling to this name.
Porkchop porkchop;

static bool handleStellaCommand(StellaLink::CommandAction action, const String& payload, String& result) {
    (void)payload;

    switch (action) {
        case StellaLink::CommandAction::SNIFF:
            // Remote "Sniff" deliberately enters passive wardriving, not an active attack mode.
            porkchop.setMode(PorkchopMode::WARHOG_MODE);
            StellaLink::setTechnicalState("scanning");
            result = "Sniffing: passive WARHOG wardrive started.";
            return true;

        case StellaLink::CommandAction::STAY:
            porkchop.setMode(PorkchopMode::IDLE);
            StellaLink::setTechnicalState("paused");
            result = "Stay: current remote work paused.";
            return true;

        case StellaLink::CommandAction::HEEL:
            porkchop.setMode(PorkchopMode::IDLE);
            StellaLink::setTechnicalState("idle");
            result = "Heel: returned to idle.";
            return true;

        case StellaLink::CommandAction::BARK:
            // Temporary synthesized bark until the full Stella sound bank lands.
            SFX::tone(220, 70);
            result = "Woof.";
            return true;

        case StellaLink::CommandAction::FETCH:
            result = "Fetch transport is not wired yet; protocol path is reserved.";
            return false;

        case StellaLink::CommandAction::DELIVER:
            result = "Deliver transport is not wired yet; protocol path is reserved.";
            return false;

        case StellaLink::CommandAction::DISPLAY_SNAPSHOT:
            result = "Display snapshot transport is not wired yet.";
            return false;

        case StellaLink::CommandAction::LOGS_TAIL:
            result = "Remote log tail is not wired yet.";
            return false;

        case StellaLink::CommandAction::FIRMWARE_PREPARE:
            result = "Remote firmware staging is not wired yet.";
            return false;

        case StellaLink::CommandAction::REBOOT:
            result = "Remote reboot intentionally disabled until authenticated pairing lands.";
            return false;

        default:
            result = "Unknown Stella command.";
            return false;
    }
}

static void updateStellaTechnicalState() {
    switch (porkchop.getMode()) {
        case PorkchopMode::WARHOG_MODE:
        case PorkchopMode::DNH_MODE:
        case PorkchopMode::OINK_MODE:
        case PorkchopMode::SPECTRUM_MODE:
            StellaLink::setTechnicalState("scanning");
            break;
        case PorkchopMode::PIGSYNC_DEVICE_SELECT:
        case PorkchopMode::PIGSYNC_CALL:
        case PorkchopMode::FILE_TRANSFER:
            StellaLink::setTechnicalState("syncing");
            break;
        case PorkchopMode::CHARGING:
            StellaLink::setTechnicalState("sleeping");
            break;
        default:
            StellaLink::setTechnicalState("idle");
            break;
    }
}

// Pre-init WiFi driver early to avoid later esp_wifi_init() failures after heap fragmentation.
static void preInitWiFiDriverEarly() {
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true /* wifioff */, false /* eraseap */);
    WiFi.setSleep(false);
    delay(HeapPolicy::kWiFiModeDelayMs);
}

// Reservation Fence: Force WiFi driver allocations to the top of heap,
// leaving a large contiguous region below for application use.
static void setupHeapLayout() {
    size_t beforeFree = ESP.getFreeHeap();
    size_t beforeLargest = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    Serial.printf("[BOOT] Pre-fence heap: free=%u largest=%u\n",
                  (unsigned)beforeFree, (unsigned)beforeLargest);

    static constexpr size_t kFenceSize = 80000;
    void* fence = heap_caps_malloc(kFenceSize, MALLOC_CAP_8BIT);
    if (fence) {
        Serial.printf("[BOOT] Fence allocated: %u bytes at %p\n",
                      (unsigned)kFenceSize, fence);
    } else {
        Serial.println("[BOOT] WARNING: Fence allocation failed, falling back to direct init");
    }

    preInitWiFiDriverEarly();

    if (fence) heap_caps_free(fence);

    size_t afterFree = ESP.getFreeHeap();
    size_t afterLargest = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    Serial.printf("[BOOT] Post-fence heap: free=%u largest=%u\n",
                  (unsigned)afterFree, (unsigned)afterLargest);
}

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("\n=== STELLA WAKING UP ===");
    Serial.printf("Device: %s | FW: %s | Protocol: v%u\n",
                  StellaIdentity::kDeviceName,
                  StellaIdentity::firmwareVersion(),
                  StellaIdentity::kProtocolVersion);

    // Deassert CapLoRa SX1262 CS BEFORE SD init. The SX1262 shares
    // MOSI(G14)/MISO(G39)/SCK(G40) with the SD card.
    pinMode(5, OUTPUT);
    digitalWrite(5, HIGH);

    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);

    pinMode(0, INPUT_PULLUP);

    setupHeapLayout();

    if (!Config::init()) {
        Serial.println("[MAIN] Config init failed, using defaults");
    }

    SDLog::init();
    HeapHealth::loadPreviousSession();

    Display::init();
    SFX::init();

    // Legacy splash renderer is retained temporarily; the Stella art pass will
    // replace the old pig-specific frames rather than layering more hacks here.
    Display::showBootSplash();

    M5.Display.setBrightness(Config::personality().brightness * 255 / 100);

    // Legacy personality engine is still called Avatar/Mood internally during migration.
    Avatar::init();
    Mood::init();

    if (Config::gps().enabled) {
        if (Config::gps().source == GPSSource::CAP_LORA) {
            auto board = M5.getBoard();
            if (board != m5::board_t::board_M5CardputerADV) {
                Serial.println("[GPS] WARNING: Cap LoRa868 GPS selected but hardware is not Cardputer ADV!");
                Serial.println("[GPS] Cap LoRa868 requires Cardputer ADV EXT bus. Check config.");
            }
            Config::prepareCapLoraGpio();
        }
        GPS::init(Config::gps().rxPin, Config::gps().txPin, Config::gps().baudRate);

        if (Config::gps().source == GPSSource::CAP_LORA) {
            Serial.println("[GPS] Re-verifying SD card after CapLoRa GPS UART init...");
            if (!Config::reinitSD()) {
                Serial.println("[GPS] WARNING: SD card re-init failed after CapLoRa GPS init");
            }
        }
    }

    OinkMode::init();
    WarhogMode::init();
    porkchop.init();

    Serial.println("=== STELLA READY ===");
    Serial.printf("Wardog: %s\n", Config::personality().name);

    Serial.printf("[DBG-HEAP] After init: free=%u largest=%u\n",
                  (unsigned)ESP.getFreeHeap(),
                  (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));

    NetworkRecon::start();

    // W33Z integration is opt-in through /stella_link.json. With no config,
    // the original offline firmware behavior remains untouched.
    StellaLink::init(handleStellaCommand);

    HeapHealth::resetPeaks(true);
}

void loop() {
    M5Cardputer.update();

    static uint32_t lastHeapLog = 0;
    if (millis() - lastHeapLog > 5000) {
        lastHeapLog = millis();
        Serial.printf("[DBG-HEAP-LOOP] free=%u largest=%u minFree=%u\n",
                      (unsigned)ESP.getFreeHeap(),
                      (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT),
                      (unsigned)ESP.getMinFreeHeap());
    }

    HeapHealth::persistWatermarks();

    {
        static bool bakedActive = false;
        static uint32_t bakedStartMs = 0;
        static uint32_t bakedDurationMs = 0;
        static bool bakedTriggered = false;
        static uint32_t lastBakedCheck = 0;

        if (bakedActive) {
            if (millis() - bakedStartMs >= bakedDurationMs) {
                bakedActive = false;
            } else {
                yield();
                return;
            }
        }

        if (!bakedTriggered && XP::hasUnlockable(3) && millis() - lastBakedCheck > 1000) {
            lastBakedCheck = millis();
            time_t now = time(nullptr);
            if (now > 1600000000) {
                int8_t tzOffset = Config::gps().timezoneOffset;
                now += (int32_t)tzOffset * 3600;
                struct tm timeinfo;
                gmtime_r(&now, &timeinfo);
                if ((timeinfo.tm_hour == 4 || timeinfo.tm_hour == 16) && timeinfo.tm_min == 20) {
                    bakedActive = true;
                    bakedStartMs = millis();
                    bakedDurationMs = random(120000, 420001);
                    bakedTriggered = true;
                }
            }
        }
    }

    if (Config::gps().enabled) GPS::update();

    Mood::update();
    porkchop.update();
    updateStellaTechnicalState();

    // Keeps Wi-Fi telemetry/heartbeat and W33Z command polling non-blocking.
    StellaLink::update();

    Display::update();
}
