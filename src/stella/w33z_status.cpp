#include "w33z_link.h"

#include <WiFi.h>

#include "../ui/display.h"

namespace StellaLink {

void showSyncStatus() {
    // Radio ownership is maintained continuously inside StellaLink::update().
    // Keeping it there avoids a second FreeRTOS task and guarantees that recon
    // cannot silently resume while the W33Z Sync screen is still active.
    nudge();

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
