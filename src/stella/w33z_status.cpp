#include "w33z_link.h"

#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "../core/network_recon.h"
#include "../core/porkchop.h"
#include "../ui/display.h"

extern Porkchop porkchop;

namespace StellaLink {
namespace {

TaskHandle_t syncRadioLeaseTaskHandle = nullptr;
volatile bool reconPausedForW33z = false;

void syncRadioLeaseTask(void*) {
    // The ESP32-S3 has a single 2.4 GHz Wi-Fi radio. Background recon normally
    // hops channels, which cannot coexist with a stable infrastructure Wi-Fi
    // association. While the on-device W33Z Sync screen is open, temporarily
    // park recon so Stella can keep its authenticated control-plane link alive.
    while (porkchop.getMode() == PorkchopMode::PIGSYNC_DEVICE_SELECT) {
        if (!reconPausedForW33z && NetworkRecon::isRunning()) {
            NetworkRecon::pause();
            reconPausedForW33z = true;
            nudge();
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Give an in-flight command result / telemetry POST a short grace period
    // before restoring channel hopping. If the user re-enters W33Z Sync during
    // that window, keep the lease instead of bouncing the radio.
    vTaskDelay(pdMS_TO_TICKS(1200));

    if (porkchop.getMode() != PorkchopMode::PIGSYNC_DEVICE_SELECT && reconPausedForW33z) {
        if (NetworkRecon::isPaused()) {
            NetworkRecon::resume();
        }
        reconPausedForW33z = false;
    }

    syncRadioLeaseTaskHandle = nullptr;
    vTaskDelete(nullptr);
}

void ensureSyncRadioLease() {
    if (syncRadioLeaseTaskHandle) return;

    BaseType_t created = xTaskCreate(
        syncRadioLeaseTask,
        "w33z-radio",
        2048,
        nullptr,
        1,
        &syncRadioLeaseTaskHandle
    );

    if (created != pdPASS) {
        syncRadioLeaseTaskHandle = nullptr;
    }
}

} // namespace

void showSyncStatus() {
    ensureSyncRadioLease();

    String top;
    String body = "W33Z SYNC\n";

    if (!isPaired()) {
        top = "W33Z: USB PAIR REQUIRED";
        body += "STATUS: UNPAIRED\n";
        body += "CONNECT USB TO W33Z\n";
        body += "DEVICE CONSOLE -> CONNECT STELLA";
    } else if (isRegistered() && WiFi.status() == WL_CONNECTED) {
        top = "W33Z: LINKED";
        body += "STATUS: LINKED\n";
        body += "WIFI: CONNECTED\n";
        body += deviceId();
    } else if (WiFi.status() == WL_CONNECTED) {
        top = "W33Z: CONNECTING";
        body += "STATUS: PAIRED\n";
        body += "SERVER: CONNECTING\n";
        body += deviceId();
    } else {
        top = "W33Z: WAITING WIFI";
        body += "STATUS: PAIRED\n";
        body += "WIFI: WAITING\n";
        body += deviceId();
    }

    Display::setTopBarMessage(top.c_str(), 0);
    Display::showToast(body.c_str(), 4000);
}

} // namespace StellaLink
