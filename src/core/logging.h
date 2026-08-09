#pragma once

// Stella is migrating the original Porkchop core incrementally. New code uses
// STELLA_LOG_ENABLED; legacy translation units still understand the old macro.
#ifndef STELLA_LOG_ENABLED
  #ifdef PORKCHOP_LOG_ENABLED
    #define STELLA_LOG_ENABLED PORKCHOP_LOG_ENABLED
  #else
    #define STELLA_LOG_ENABLED 1
  #endif
#endif

#ifndef PORKCHOP_LOG_ENABLED
#define PORKCHOP_LOG_ENABLED STELLA_LOG_ENABLED
#endif

#if !STELLA_LOG_ENABLED
#ifdef __cplusplus
#ifdef ARDUINO
#include <Arduino.h>
#endif

#if !defined(ARDUINO_CORE_BUILD)
// Release builds suppress formatted diagnostic chatter, but Serial is also a
// real bidirectional transport on ESP32-S3. The previous pure null sink dropped
// RX and TX as well, which made W33Z USB enrollment impossible and caused the
// host to block because the USB Serial/JTAG RX queue was never drained.
//
// Keep print/printf-style logs compiled out, while forwarding lifecycle,
// raw-byte I/O and empty println() framing to the actual Arduino Serial object.
// This preserves the small/quiet release build without hijacking the transport.
struct StellaNullSerial {
    void begin(unsigned long baud, uint8_t = 0) {
#ifdef ARDUINO
        ::Serial.begin(baud);
#else
        (void)baud;
#endif
    }

    void end() {
#ifdef ARDUINO
        ::Serial.end();
#endif
    }

    void flush() {
#ifdef ARDUINO
        ::Serial.flush();
#endif
    }

    void setTimeout(unsigned long timeout) {
#ifdef ARDUINO
        ::Serial.setTimeout(timeout);
#else
        (void)timeout;
#endif
    }

    unsigned long getTimeout() {
#ifdef ARDUINO
        return ::Serial.getTimeout();
#else
        return 0;
#endif
    }

    void setDebugOutput(bool enabled) {
#ifdef ARDUINO
        ::Serial.setDebugOutput(enabled);
#else
        (void)enabled;
#endif
    }

    int available() {
#ifdef ARDUINO
        return ::Serial.available();
#else
        return 0;
#endif
    }

    int read() {
#ifdef ARDUINO
        return ::Serial.read();
#else
        return -1;
#endif
    }

    int peek() {
#ifdef ARDUINO
        return ::Serial.peek();
#else
        return -1;
#endif
    }

    template <typename... Args>
    size_t printf(const char*, Args...) { return 0; }

    template <typename T>
    size_t print(const T&) { return 0; }

    size_t print(const char*) { return 0; }
    size_t print(char) { return 0; }

    template <typename T>
    size_t println(const T&) { return 0; }

    // JSON-over-USB uses serializeJson(..., Serial) followed by an empty
    // println() as its line delimiter. Forward only this framing newline.
    size_t println() {
#ifdef ARDUINO
        return ::Serial.println();
#else
        return 0;
#endif
    }

    size_t write(uint8_t value) {
#ifdef ARDUINO
        return ::Serial.write(value);
#else
        (void)value;
        return 1;
#endif
    }

    size_t write(const uint8_t* data, size_t size) {
#ifdef ARDUINO
        return ::Serial.write(data, size);
#else
        (void)data;
        return size;
#endif
    }

    operator bool() const {
#ifdef ARDUINO
        return static_cast<bool>(::Serial);
#else
        return false;
#endif
    }
};

static StellaNullSerial StellaSerialSink;
#undef Serial
#define Serial StellaSerialSink
#endif
#endif
#endif
