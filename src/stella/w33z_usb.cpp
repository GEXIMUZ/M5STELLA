#include "w33z_link.h"

#include <Arduino.h>

namespace StellaLink {

void initUsbTransport() {
    // The StampS3 board uses ESP32-S3's fixed-function USB Serial/JTAG CDC
    // controller (ARDUINO_USB_MODE=1). Serial.begin() initializes its RX/TX
    // queues and interrupts; serviceUsbBootstrap() drains RX from the main loop.
    // No TinyUSB stack or cross-task RX callback is needed.
    Serial.begin(115200);
    Serial.setTimeout(50);
}

} // namespace StellaLink
