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

namespace Groups {
static constexpr const char* Assess = "ASSESS";
static constexpr const char* Recon = "RECON";
static constexpr const char* Captures = "CAPTURES";
static constexpr const char* Pack = "PACK";
static constexpr const char* Status = "STATUS";
static constexpr const char* System = "SYSTEM";
}

namespace Modes {
static constexpr const char* Bite = "WIFI ATTACK";
static constexpr const char* Sniff = "PASSIVE WIFI";
static constexpr const char* Patrol = "WARDRIVING";
static constexpr const char* BluePaws = "BLE TOOLS";
static constexpr const char* Airwatch = "SPECTRUM";
static constexpr const char* PackLink = "W33Z SYNC";
static constexpr const char* Howl = "RF BEACON";
static constexpr const char* FriendlyPack = "TRUSTED NETS";
static constexpr const char* WardogStats = "STATS";
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

// Compact user-facing aliases for UI modules. This keeps copy centralized while
// donor subsystem class names are migrated independently.
namespace StellaLanguage {
static constexpr const char* kBite = StellaIdentity::Modes::Bite;
static constexpr const char* kSniff = StellaIdentity::Modes::Sniff;
static constexpr const char* kPatrol = StellaIdentity::Modes::Patrol;
static constexpr const char* kBluePaws = StellaIdentity::Modes::BluePaws;
static constexpr const char* kAirwatch = StellaIdentity::Modes::Airwatch;
static constexpr const char* kPackLink = StellaIdentity::Modes::PackLink;
static constexpr const char* kHowl = StellaIdentity::Modes::Howl;
static constexpr const char* kFriendlyPack = StellaIdentity::Modes::FriendlyPack;
static constexpr const char* kWardogStats = StellaIdentity::Modes::WardogStats;
static constexpr const char* kAboutStella = StellaIdentity::Modes::About;
}
