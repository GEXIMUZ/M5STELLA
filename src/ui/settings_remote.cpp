#include "settings_menu.h"

#include <M5Cardputer.h>
#include <string.h>

#include "../core/config.h"
#include "../core/sdlog.h"

namespace {

static int clampRemote(int value, int minValue, int maxValue) {
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

static uint8_t remoteGroupCount(uint8_t group) {
    switch (group) {
        case 1: return 2;   // NETWORK
        case 2: return 5;   // INTEGRATION
        case 3: return 11;  // RADIO
        case 4: return 8;   // GPS
        case 5: return 2;   // BLE
        case 6: return 1;   // LOG (legacy/internal)
        default: return 0;
    }
}

} // namespace

void SettingsMenu::handleRemoteAction(const char* action) {
    if (!active || !action || !*action) return;

    const bool up = strcmp(action, "up") == 0;
    const bool down = strcmp(action, "down") == 0;
    const bool enter = strcmp(action, "enter") == 0;
    const bool back = strcmp(action, "back") == 0;
    if (!up && !down && !enter && !back) return;

    // Remote v1 deliberately does not type into secret/text fields. Browsing,
    // toggles and numeric settings mirror the physical controls.
    if (textEditing) {
        if (back) {
            textEditing = false;
            textBuffer[0] = '\0';
            textLen = 0;
        }
        return;
    }

    lastInputMs = millis();

    auto markPersonality = []() {
        SettingsMenu::dirtyPersonality = true;
        SettingsMenu::lastInputMs = millis();
    };
    auto markConfig = []() {
        SettingsMenu::dirtyConfig = true;
        SettingsMenu::lastInputMs = millis();
    };

    // VALUE editing: up increases, down decreases, matching the physical UI.
    if ((up || down) && editing) {
        const int dir = up ? 1 : -1;

        if (activeGroup == 0) {
            switch (rootIndex) {
                case 0: { // THEME
                    int next = clampRemote((int)Config::personality().themeIndex + dir, 0, 14);
                    if (next != Config::personality().themeIndex) {
                        Config::personality().themeIndex = (uint8_t)next;
                        markPersonality();
                    }
                    break;
                }
                case 1: { // BRIGHTNESS
                    int next = clampRemote((int)Config::personality().brightness + dir * 10, 10, 100);
                    if (next != Config::personality().brightness) {
                        Config::personality().brightness = (uint8_t)next;
                        M5.Display.setBrightness((uint8_t)next * 255 / 100);
                        markPersonality();
                    }
                    break;
                }
                case 3: { // DIM AFTER
                    int next = clampRemote((int)Config::personality().dimTimeout + dir * 10, 0, 300);
                    if (next != Config::personality().dimTimeout) {
                        Config::personality().dimTimeout = (uint16_t)next;
                        markPersonality();
                    }
                    break;
                }
                case 4: { // DIM LEVEL
                    int next = clampRemote((int)Config::personality().dimLevel + dir * 5, 0, 50);
                    if (next != Config::personality().dimLevel) {
                        Config::personality().dimLevel = (uint8_t)next;
                        markPersonality();
                    }
                    break;
                }
                case 5: { // G0 ACTION
                    int next = clampRemote((int)Config::personality().g0Action + dir, 0, G0_ACTION_COUNT - 1);
                    if (next != (int)Config::personality().g0Action) {
                        Config::personality().g0Action = (G0Action)next;
                        markPersonality();
                    }
                    break;
                }
                case 6: { // BOOT MODE
                    int next = clampRemote((int)Config::personality().bootMode + dir, 0, BOOT_MODE_COUNT - 1);
                    if (next != (int)Config::personality().bootMode) {
                        Config::personality().bootMode = (BootMode)next;
                        markPersonality();
                    }
                    break;
                }
                default:
                    editing = false;
                    break;
            }
            return;
        }

        if (activeGroup == 3) { // RADIO
            WiFiConfig& wifi = Config::wifi();
            switch (groupIndex) {
                case 0: {
                    int next = clampRemote((int)wifi.channelHopInterval + dir * 50, 50, 2000);
                    if (next != wifi.channelHopInterval) { wifi.channelHopInterval = (uint16_t)next; markConfig(); }
                    break;
                }
                case 1: {
                    int next = clampRemote((int)wifi.spectrumHopInterval + dir * 50, 50, 2000);
                    if (next != wifi.spectrumHopInterval) { wifi.spectrumHopInterval = (uint16_t)next; markConfig(); }
                    break;
                }
                case 3: {
                    int next = clampRemote((int)wifi.lockTime + dir * 500, 1000, 10000);
                    if (next != wifi.lockTime) { wifi.lockTime = (uint16_t)next; markConfig(); }
                    break;
                }
                case 6: {
                    int next = clampRemote((int)wifi.attackMinRssi + dir * 5, -90, -50);
                    if (next != wifi.attackMinRssi) { wifi.attackMinRssi = (int8_t)next; markConfig(); }
                    break;
                }
                case 7: {
                    int next = clampRemote((int)wifi.spectrumMinRssi + dir * 5, -95, -30);
                    if (next != wifi.spectrumMinRssi) { wifi.spectrumMinRssi = (int8_t)next; markConfig(); }
                    break;
                }
                case 8: {
                    int next = clampRemote((int)wifi.spectrumTopN + dir * 5, 0, 100);
                    if (next != wifi.spectrumTopN) { wifi.spectrumTopN = (uint8_t)next; markConfig(); }
                    break;
                }
                case 9: {
                    int seconds = clampRemote((int)(wifi.spectrumStaleMs / 1000) + dir, 1, 20);
                    uint16_t next = (uint16_t)(seconds * 1000);
                    if (next != wifi.spectrumStaleMs) { wifi.spectrumStaleMs = next; markConfig(); }
                    break;
                }
                default:
                    editing = false;
                    break;
            }
            return;
        }

        if (activeGroup == 4) { // GPS
            GPSConfig& gps = Config::gps();
            switch (groupIndex) {
                case 1: {
                    int next = clampRemote((int)gps.source + dir, 0, GPS_SOURCE_COUNT - 1);
                    if (next != (int)gps.source) {
                        gps.source = (GPSSource)next;
                        if (gps.source == GPSSource::GROVE) { gps.rxPin = 1; gps.txPin = 2; }
                        else if (gps.source == GPSSource::CAP_LORA) { gps.rxPin = CapLoraPins::GPS_RX; gps.txPin = CapLoraPins::GPS_TX; }
                        markConfig();
                    }
                    break;
                }
                case 3: {
                    int next = clampRemote((int)gps.updateInterval + dir, 1, 30);
                    if (next != gps.updateInterval) { gps.updateInterval = (uint16_t)next; markConfig(); }
                    break;
                }
                case 4: {
                    static const uint32_t rates[] = {9600, 38400, 57600, 115200};
                    int index = 3;
                    for (int i = 0; i < 4; ++i) if (gps.baudRate == rates[i]) index = i;
                    int next = clampRemote(index + dir, 0, 3);
                    if (rates[next] != gps.baudRate) { gps.baudRate = rates[next]; markConfig(); }
                    break;
                }
                case 5: {
                    int next = clampRemote((int)gps.rxPin + dir, 1, 46);
                    if (next != gps.rxPin) { gps.rxPin = (uint8_t)next; markConfig(); }
                    break;
                }
                case 6: {
                    int next = clampRemote((int)gps.txPin + dir, 1, 46);
                    if (next != gps.txPin) { gps.txPin = (uint8_t)next; markConfig(); }
                    break;
                }
                case 7: {
                    int next = clampRemote((int)gps.timezoneOffset + dir, -12, 14);
                    if (next != gps.timezoneOffset) { gps.timezoneOffset = (int8_t)next; markConfig(); }
                    break;
                }
                default:
                    editing = false;
                    break;
            }
            return;
        }

        if (activeGroup == 5) { // BLE
            BLEConfig& ble = Config::ble();
            if (groupIndex == 0) {
                int next = clampRemote((int)ble.burstInterval + dir * 50, 50, 500);
                if (next != ble.burstInterval) { ble.burstInterval = (uint16_t)next; markConfig(); }
            } else if (groupIndex == 1) {
                int next = clampRemote((int)ble.advDuration + dir * 25, 50, 200);
                if (next != ble.advDuration) { ble.advDuration = (uint16_t)next; markConfig(); }
            } else {
                editing = false;
            }
            return;
        }

        editing = false;
        return;
    }

    if (up || down) {
        if (activeGroup == 0) {
            const uint8_t rootCount = 13;
            if (up && rootIndex > 0) rootIndex--;
            if (down && rootIndex + 1 < rootCount) rootIndex++;

            if (rootIndex < rootScroll) rootScroll = rootIndex;
            else if (rootIndex >= rootScroll + VISIBLE_ROOT_ITEMS) rootScroll = rootIndex - VISIBLE_ROOT_ITEMS + 1;
        } else {
            uint8_t count = remoteGroupCount(activeGroup);
            if (count == 0) return;
            if (up && groupIndex > 0) groupIndex--;
            if (down && groupIndex + 1 < count) groupIndex++;

            if (groupIndex < groupScroll) groupScroll = groupIndex;
            else if (groupIndex >= groupScroll + VISIBLE_GROUP_ITEMS) groupScroll = groupIndex - VISIBLE_GROUP_ITEMS + 1;
        }
        editing = false;
        return;
    }

    if (enter) {
        if (activeGroup == 0) {
            if (rootIndex >= 8 && rootIndex <= 12) {
                activeGroup = (uint8_t)(rootIndex - 7); // NETWORK..BLE => 1..5
                groupIndex = 0;
                groupScroll = 0;
                editing = false;
                return;
            }

            if (rootIndex == 2) { // SOUND
                Config::personality().soundEnabled = !Config::personality().soundEnabled;
                markPersonality();
                return;
            }

            if (rootIndex == 0 || rootIndex == 1 || rootIndex == 3 || rootIndex == 4 || rootIndex == 5 || rootIndex == 6) {
                editing = !editing;
            }
            // C4LLS1GN is intentionally browse-only remotely in v1.
            return;
        }

        if (activeGroup == 2) { // INTEGRATION actions only; secrets remain local-only.
            if (groupIndex == 1) Config::loadWpaSecKeyFromFile();
            else if (groupIndex == 4) Config::loadWigleKeyFromFile();
            return;
        }

        if (activeGroup == 3) { // RADIO toggles / values
            WiFiConfig& wifi = Config::wifi();
            switch (groupIndex) {
                case 2: wifi.spectrumTiltEnabled = !wifi.spectrumTiltEnabled; markConfig(); return;
                case 4: wifi.enableDeauth = !wifi.enableDeauth; markConfig(); return;
                case 5: wifi.randomizeMAC = !wifi.randomizeMAC; markConfig(); return;
                case 10: wifi.spectrumCollapseSsid = !wifi.spectrumCollapseSsid; markConfig(); return;
                default: editing = !editing; return;
            }
        }

        if (activeGroup == 4) { // GPS toggles / values
            GPSConfig& gps = Config::gps();
            if (groupIndex == 0) { gps.enabled = !gps.enabled; markConfig(); return; }
            if (groupIndex == 2) { gps.powerSave = !gps.powerSave; markConfig(); return; }
            editing = !editing;
            return;
        }

        if (activeGroup == 5) { // BLE values
            editing = !editing;
            return;
        }

        // NETWORK/text settings are browse-only remotely in v1.
        return;
    }

    if (back) {
        if (editing) {
            editing = false;
        } else if (activeGroup != 0) {
            activeGroup = 0;
            groupIndex = 0;
            groupScroll = 0;
        } else {
            saveIfDirty(true);
            exitRequested = true;
        }
    }
}
