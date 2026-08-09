#include "w33z_link.h"

#include <Arduino.h>

namespace StellaLink {

void initUsbTransport() {
    // Stella uses ESP32-S3 native USB CDC (ARDUINO_USB_MODE=0) for the W33Z
    // browser bootstrap and live console. Serial remains the protocol stream;
    // serviceUsbBootstrap() drains newline-delimited requests in the main loop.
    Serial.begin(115200);
    Serial.setTimeout(50);
}

} // namespace StellaLink