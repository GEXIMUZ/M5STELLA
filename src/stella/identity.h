#pragma once

#include <Arduino.h>
#include "../build_info.h"

namespace StellaIdentity {

static constexpr uint16_t kProtocolVersion = 1;
static constexpr const char* kDeviceName = "Stella";
static constexpr const char* kModel = "M5STELLA Wardog";
static constexpr const char* kHardwareRevision = "M5Cardputer-ESP32S3";
static constexpr const char* kProduct = "M5STELLA";
static constexpr const char* kTitle = "STELLA THE WARDOG";
static constexpr const char* kPlatform = "W33Z";

// These names intentionally match the W33Z Stella protocol exactly.
static constexpr const char* kCapabilities[] = {
    "telemetry",
    "gps",
    "wifi_scan",
    "capture_inventory",
    "file_sync",
    "display_mirror",
    "display_input",
    "firmware_update",
    "device_logs",
    "ble_provisioning"
};

static constexpr size_t kCapabilityCount = sizeof(kCapabilities) / sizeof(kCapabilities[0]);

// User-facing vocabulary. Internal donor class names remain compatible until
// each subsystem has been migrated and compile-tested.
namespace Groups {
static constexpr const char* Assess = "ASSESS";
static constexpr const char* Recon = "RECON";
static constexpr const char* Captures = "CAPTURES";
static constexpr const char* Pack = "PACK";
static constexpr const char* Status = "STATUS";
static constexpr const char* System = "SYSTEM";
}

namespace Modes {
static constexpr const char* Bite = "BITE";                 // authorized active assessment
static constexpr const char* Sniff = "SNIFF";               // passive Wi-Fi discovery
static constexpr const char* Patrol = "PATROL";             // GPS wardriving
static constexpr const char* BluePaws = "BLUE PAWS";        // BLE reconnaissance
static constexpr const char* Airwatch = "AIRWATCH";         // RF/spectrum inspection
static constexpr const char* PackLink = "PACK LINK";        // W33Z sync + control link
static constexpr const char* Howl = "HOWL";                 // RF transmit tooling
static constexpr const char* FriendlyPack = "FRIENDLY PACK";// trusted/excluded devices
static constexpr const char* WardogStats = "WARDOG STATS";
static constexpr const char* About = "ABOUT STELLA";
}

namespace States {
static constexpr const char* Idle = "SITTING";
static constexpr const char* Scanning = "SNIFFING";
static constexpr const char* Tracking = "TRACKING";
static constexpr const char* Syncing = "FETCHING";
static constexpr const char* Connected = "ON LEASH";
static constexpr const char* Disconnected = "OFF LEASH";
static constexpr const char* Warning = "GROWLING";
static constexpr const char* Error = "HOWLING";
static constexpr const char* Sleep = "NAPPING";
}

inline const char* firmwareVersion() {
    return BUILD_VERSION;
}

inline String deviceId() {
    uint64_t mac = ESP.getEfuseMac();
    char out[32];
    snprintf(out, sizeof(out), "stella-%04X%08X",
             static_cast<uint16_t>(mac >> 32),
             static_cast<uint32_t>(mac));
    return String(out);
}

} // namespace StellaIdentity
