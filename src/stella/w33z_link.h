#pragma once

#include <Arduino.h>

namespace StellaLink {

enum class CommandAction : uint8_t {
    NONE = 0,
    SNIFF,
    STAY,
    HEEL,
    FETCH,
    DELIVER,
    BARK,
    REBOOT,
    DISPLAY_SNAPSHOT,
    LOGS_TAIL,
    FIRMWARE_PREPARE
};

using CommandHandler = bool (*)(CommandAction action, const String& payload, String& result);

// Initializes Stella's W33Z identity, stored enrollment state and optional
// advanced /stella_link.json overrides. Normal enrollment does not require an
// SD-card config file: USB bootstrap + NVS is the default path.
void init(CommandHandler handler = nullptr);

// Register the native ESP32-S3 USB CDC RX event handler and ensure the TinyUSB
// device stack is running. This follows Espressif's official USBSerial example
// for ARDUINO_USB_MODE=0 / ARDUINO_USB_CDC_ON_BOOT=1.
void initUsbTransport();

// Service the native USB CDC bootstrap protocol. This is intentionally public
// so main.cpp can run it before heavier UI/recon work on every loop iteration.
void serviceUsbBootstrap();

// Non-blocking Wi-Fi service. Before USB enrollment it performs no Wi-Fi/TLS
// work; after enrollment it handles handshake, telemetry and command polling.
void update();

bool isEnabled();
bool isConnected();
bool isRegistered();
bool isPaired();
const String& baseUrl();
const String& deviceId();
const String& lastError();

// On-device COMMS -> W33Z SYNC status view. Pairing itself remains USB-first;
// this function only reports enrollment/link state and lets the menu request a
// reconnect with nudge().
void showSyncStatus();

// Firmware can expose its current technical state without coupling W33Z to the
// internal UI/mode naming. Valid states: idle, scanning, syncing, paused,
// warning, error, sleeping.
void setTechnicalState(const char* state);

// Request an immediate authenticated handshake/telemetry cycle on next update.
void nudge();

} // namespace StellaLink
