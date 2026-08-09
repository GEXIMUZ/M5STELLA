#pragma once

#include <ArduinoJson.h>

namespace StellaDeviceConsole {

// Handle trusted, local USB console commands that are intentionally separate
// from W33Z Wi-Fi commands. Returns true when the message belonged to the
// device-console protocol and was consumed.
bool handleUsbCommand(JsonDocument& doc);

} // namespace StellaDeviceConsole
