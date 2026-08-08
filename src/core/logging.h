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
struct StellaNullSerial {
    void begin(unsigned long, uint8_t = 0) {}
    void end() {}
    void flush() {}
    void setTimeout(unsigned long) {}
    unsigned long getTimeout() { return 0; }
    void setDebugOutput(bool) {}
    int available() { return 0; }
    int read() { return -1; }
    int peek() { return -1; }

    template <typename... Args>
    size_t printf(const char*, Args...) { return 0; }

    template <typename T>
    size_t print(const T&) { return 0; }

    size_t print(const char*) { return 0; }
    size_t print(char) { return 0; }

    template <typename T>
    size_t println(const T&) { return 0; }

    size_t println() { return 0; }

    size_t write(uint8_t) { return 1; }
    size_t write(const uint8_t*, size_t n) { return n; }
    operator bool() const { return false; }
};

static StellaNullSerial StellaSerialSink;
#undef Serial
#define Serial StellaSerialSink
#endif
#endif
#endif
