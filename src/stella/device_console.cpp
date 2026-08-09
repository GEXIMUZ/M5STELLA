#include "device_console.h"

#include <M5Cardputer.h>
#include <mbedtls/base64.h>
#include <string.h>

#include "../core/porkchop.h"
#include "../ui/display.h"
#include "../ui/settings_menu.h"

extern Porkchop porkchop;

namespace StellaDeviceConsole {
namespace {

static constexpr uint8_t kMaxRowsPerChunk = 8;
static uint8_t pixelChunk[DISPLAY_W * kMaxRowsPerChunk];
static unsigned char base64Chunk[((DISPLAY_W * kMaxRowsPerChunk + 2) / 3) * 4 + 1];

void writeJsonLine(JsonDocument& doc) {
    serializeJson(doc, Serial);
    Serial.println();
}

void writeError(const char* requestType, const char* message) {
    JsonDocument reply;
    reply["type"] = "stella.console.error";
    reply["request"] = requestType ? requestType : "unknown";
    reply["error"] = message ? message : "console error";
    writeJsonLine(reply);
}

const uint8_t* rowPointer(uint16_t y) {
    if (y < TOP_BAR_H) {
        auto* buffer = static_cast<uint8_t*>(Display::getTopBar().getBuffer());
        return buffer ? buffer + (size_t)y * DISPLAY_W : nullptr;
    }

    if (y < TOP_BAR_H + MAIN_H) {
        auto* buffer = static_cast<uint8_t*>(Display::getMain().getBuffer());
        const uint16_t localY = y - TOP_BAR_H;
        return buffer ? buffer + (size_t)localY * DISPLAY_W : nullptr;
    }

    if (y < DISPLAY_H) {
        auto* buffer = static_cast<uint8_t*>(Display::getBottomBar().getBuffer());
        const uint16_t localY = y - (DISPLAY_H - BOTTOM_BAR_H);
        return buffer ? buffer + (size_t)localY * DISPLAY_W : nullptr;
    }

    return nullptr;
}

bool sendDisplayInfo() {
    JsonDocument reply;
    reply["type"] = "stella.display.info";
    reply["width"] = DISPLAY_W;
    reply["height"] = DISPLAY_H;
    reply["format"] = "rgb332";
    reply["rowsPerChunk"] = kMaxRowsPerChunk;
    writeJsonLine(reply);
    return true;
}

bool sendDisplayChunk(JsonDocument& request) {
    int y = request["y"] | -1;
    int requestedRows = request["rows"] | (int)kMaxRowsPerChunk;

    if (y < 0 || y >= DISPLAY_H) {
        writeError("display.chunk", "invalid y");
        return true;
    }

    if (requestedRows < 1) requestedRows = 1;
    if (requestedRows > kMaxRowsPerChunk) requestedRows = kMaxRowsPerChunk;
    if (y + requestedRows > DISPLAY_H) requestedRows = DISPLAY_H - y;

    const size_t rawLen = (size_t)requestedRows * DISPLAY_W;
    for (int row = 0; row < requestedRows; ++row) {
        const uint8_t* src = rowPointer((uint16_t)(y + row));
        if (!src) {
            writeError("display.chunk", "display buffer unavailable");
            return true;
        }
        memcpy(pixelChunk + (size_t)row * DISPLAY_W, src, DISPLAY_W);
    }

    size_t encodedLen = 0;
    const int rc = mbedtls_base64_encode(
        base64Chunk,
        sizeof(base64Chunk),
        &encodedLen,
        pixelChunk,
        rawLen
    );
    if (rc != 0 || encodedLen >= sizeof(base64Chunk)) {
        writeError("display.chunk", "base64 encode failed");
        return true;
    }
    base64Chunk[encodedLen] = '\0';

    JsonDocument reply;
    reply["type"] = "stella.display.chunk";
    reply["y"] = y;
    reply["rows"] = requestedRows;
    reply["width"] = DISPLAY_W;
    reply["format"] = "rgb332";
    reply["data"] = reinterpret_cast<const char*>(base64Chunk);
    writeJsonLine(reply);
    return true;
}

bool handleInput(JsonDocument& request) {
    const char* action = request["action"] | "";
    bool ok = true;
    const char* error = nullptr;

    if (strcmp(action, "settings") == 0) {
        porkchop.setMode(PorkchopMode::SETTINGS);
    } else if (strcmp(action, "home") == 0) {
        porkchop.setMode(PorkchopMode::IDLE);
    } else if (strcmp(action, "up") == 0 ||
               strcmp(action, "down") == 0 ||
               strcmp(action, "enter") == 0 ||
               strcmp(action, "back") == 0) {
        if (porkchop.getMode() != PorkchopMode::SETTINGS || !SettingsMenu::isActive()) {
            ok = false;
            error = "open settings first";
        } else {
            SettingsMenu::handleRemoteAction(action);
            if (SettingsMenu::shouldExit()) {
                SettingsMenu::clearExit();
                porkchop.setMode(PorkchopMode::IDLE);
            }
        }
    } else {
        ok = false;
        error = "unsupported input action";
    }

    Display::resetDimTimer();

    JsonDocument reply;
    reply["type"] = "stella.input";
    reply["success"] = ok;
    reply["action"] = action;
    reply["mode"] = (uint8_t)porkchop.getMode();
    if (!ok) reply["error"] = error;
    writeJsonLine(reply);
    return true;
}

} // namespace

bool handleUsbCommand(JsonDocument& doc) {
    const char* type = doc["type"] | "";

    if (strcmp(type, "display.info") == 0) {
        return sendDisplayInfo();
    }
    if (strcmp(type, "display.chunk") == 0) {
        return sendDisplayChunk(doc);
    }
    if (strcmp(type, "input") == 0) {
        return handleInput(doc);
    }

    return false;
}

} // namespace StellaDeviceConsole
