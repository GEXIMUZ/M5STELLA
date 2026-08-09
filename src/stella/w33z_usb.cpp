#include "w33z_link.h"

#include <Arduino.h>
#include <USB.h>

namespace {

// Espressif's Arduino-ESP32 USBSerial example reads CDC RX from inside the
// ARDUINO_USB_CDC_RX_EVENT callback. Polling Serial.available() alone proved
// unreliable on the StampS3 in native USB-OTG mode, so use the supported event
// path and immediately drain the CDC RX FIFO through StellaLink's parser.
static void usbCdcEventCallback(void* arg,
                                esp_event_base_t eventBase,
                                int32_t eventId,
                                void* eventData) {
    (void)arg;
    (void)eventData;

    if (eventBase != ARDUINO_USB_CDC_EVENTS) return;
    if (eventId != ARDUINO_USB_CDC_RX_EVENT) return;

    StellaLink::serviceUsbBootstrap();
}

} // namespace

namespace StellaLink {

void initUsbTransport() {
#if ARDUINO_USB_MODE == 0
    // With ARDUINO_USB_CDC_ON_BOOT=1, Serial is the native USBCDC instance.
    // Register before servicing W33Z requests, then make sure TinyUSB is active.
    Serial.onEvent(usbCdcEventCallback);
    USB.begin();
    Serial.println("[STELLA] Native USB CDC RX event transport ready.");
#else
    Serial.println("[STELLA] WARNING: W33Z USB bootstrap requires ARDUINO_USB_MODE=0.");
#endif
}

} // namespace StellaLink
