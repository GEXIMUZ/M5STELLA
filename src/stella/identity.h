#pragma once

#include <Arduino.h>
#include "../build_info.h"

namespace StellaIdentity {

static constexpr uint16_t kProtocolVersion = 1;
static constexpr const char* kDeviceName = "Stella";
static constexpr const char* kModel = "M5STELLA Wardog";
static constexpr const char* kHardwareRevision = "M5Cardputer-ESP32S3";

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

inline const char* firmwareVersion() {
    return BUILD_VERSION;
}

inline String deviceId() {
    // Stable per-board ID derived from the ESP32 eFuse MAC.
    uint64_t mac = ESP.getEfuseMac();
    char out[32];
    snprintf(out, sizeof(out), "stella-%04X%08X",
             static_cast<uint16_t>(mac >> 32),
             static_cast<uint32_t>(mac));
    return String(out);
}

} // namespace StellaIdentity
