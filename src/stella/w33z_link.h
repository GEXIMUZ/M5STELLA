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

// Reads optional /stella_link.json from SD and prepares the W33Z client.
// The link is opt-in and disabled when no config file exists.
void init(CommandHandler handler = nullptr);

// Non-blocking periodic service. Handles heartbeat/telemetry and command polling.
void update();

bool isEnabled();
bool isConnected();
bool isRegistered();
const String& baseUrl();
const String& deviceId();
const String& lastError();

// Request an immediate handshake/telemetry cycle on next update().
void nudge();

} // namespace StellaLink
