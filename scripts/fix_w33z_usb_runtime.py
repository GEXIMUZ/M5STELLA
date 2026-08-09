#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def must_replace(path: Path, old: str, new: str, label: str):
    text = path.read_text()
    if old not in text:
        raise SystemExit(f"{label}: source pattern not found in {path}")
    path.write_text(text.replace(old, new, 1))


def replace_if_present(path: Path, old: str, new: str):
    text = path.read_text()
    if old in text:
        path.write_text(text.replace(old, new, 1))

# First apply the existing USB migration if the source tree is still baseline.
usb_script = ROOT / "scripts/enable_usb_w33z_pairing.py"
h = ROOT / "src/stella/w33z_link.h"
if "showSyncStatus" not in h.read_text():
    ns = {}
    exec(compile(usb_script.read_text(), str(usb_script), "exec"), ns, ns)

h = ROOT / "src/stella/w33z_link.h"
cpp = ROOT / "src/stella/w33z_link.cpp"
main = ROOT / "src/main.cpp"
display = ROOT / "src/ui/display.cpp"

# Public USB service: this lets main.cpp service Web Serial immediately every loop.
replace_if_present(
    h,
    "void update();\n",
    "void update();\n\n// Service USB bootstrap immediately. Safe to call every loop.\nvoid serviceUsb();\n",
)

# Rename internal service and expose a public wrapper.
text = cpp.read_text()
if "void serviceUsbBootstrap()" in text:
    text = text.replace("void serviceUsbBootstrap()", "void serviceUsbBootstrapInternal()", 1)
    text = text.replace("    serviceUsbBootstrap();\n", "    serviceUsbBootstrapInternal();\n", 1)
if "void serviceUsb()" not in text:
    marker = "void update() {\n"
    if marker not in text:
        raise SystemExit("w33z_link.cpp: update() marker not found")
    text = text.replace(marker, "void serviceUsb() { serviceUsbBootstrapInternal(); }\n\n" + marker, 1)
cpp.write_text(text)

# Service USB before the legacy state machine has a chance to do anything expensive.
replace_if_present(
    main,
    "void loop() {\n    M5Cardputer.update();\n",
    "void loop() {\n    M5Cardputer.update();\n    StellaLink::serviceUsb();\n",
)

# Display must stop invoking the old PigSync renderer for this legacy enum slot.
text = display.read_text()
if '#include "../stella/w33z_link.h"' not in text:
    text = text.replace('#include "../modes/charging.h"\n', '#include "../modes/charging.h"\n#include "../stella/w33z_link.h"\n', 1)

text = text.replace(
    '''        case PorkchopMode::PIGSYNC_DEVICE_SELECT:\n            // Draw device selection menu\n            drawPigSyncDeviceSelect(mainCanvas);\n            break;\n''',
    '''        case PorkchopMode::PIGSYNC_DEVICE_SELECT: {\n            // Legacy enum slot, but this screen is W33Z-only now. Never call\n            // the old PigSync renderer or ESP-NOW state here.\n            mainCanvas.setTextColor(COLOR_FG);\n            mainCanvas.setTextDatum(top_center);\n            mainCanvas.setTextSize(2);\n            mainCanvas.drawString("W33Z SYNC", DISPLAY_W / 2, 8);\n            mainCanvas.setTextSize(1);\n\n            const char* status = StellaLink::isRegistered() ? "LINKED" :\n                                 StellaLink::isPaired() ? "PAIRED" : "UNPAIRED";\n            mainCanvas.drawString(status, DISPLAY_W / 2, 36);\n            mainCanvas.setTextDatum(top_left);\n\n            if (StellaLink::isRegistered()) {\n                mainCanvas.drawString("W33Z.GEXZ.BE", 18, 54);\n                mainCanvas.drawString(StellaLink::deviceId(), 18, 67);\n                mainCanvas.drawString("ENTER/R = REFRESH", 18, 82);\n            } else if (StellaLink::isPaired()) {\n                mainCanvas.drawString("USB PAIRED", 18, 54);\n                mainCanvas.drawString("WAITING FOR WIFI/W33Z", 18, 67);\n                mainCanvas.drawString("ENTER/R = RETRY", 18, 82);\n            } else {\n                mainCanvas.drawString("CONNECT STELLA VIA USB", 18, 52);\n                mainCanvas.drawString("OPEN W33Z DEVICE CONSOLE", 18, 65);\n                mainCanvas.drawString("CLICK CONNECT STELLA", 18, 78);\n            }\n            break;\n        }\n''',
    1,
)

# Explicit top-bar name for the legacy enum slot.
needle = '''        case PorkchopMode::SPECTRUM_MODE:\n            snprintf(modeBuf, sizeof(modeBuf), "AIRWATCH");\n            modeColor = COLOR_ACCENT;\n            break;\n'''
if needle in text and 'snprintf(modeBuf, sizeof(modeBuf), "W33Z SYNC")' not in text:
    text = text.replace(needle, needle + '''        case PorkchopMode::PIGSYNC_DEVICE_SELECT:\n            snprintf(modeBuf, sizeof(modeBuf), "W33Z SYNC");\n            modeColor = COLOR_SUCCESS;\n            break;\n''', 1)

# Bottom bar: no CALL/select/channel language from PigSync.
text = text.replace(
    '''    } else if (mode == PorkchopMode::PIGSYNC_DEVICE_SELECT) {\n        // PIGSYNC_DEVICE_SELECT: control hints (state shown in terminal)\n        statsStr = "ENTER=CALL UP/DN=SELECT ESC=EXIT";\n''',
    '''    } else if (mode == PorkchopMode::PIGSYNC_DEVICE_SELECT) {\n        statsStr = "ENTER/R=REFRESH  ESC=EXIT";\n''',
    1,
)
text = text.replace(
    '''    // Right: uptime or PIGSYNC channel\n    bottomBar.setTextDatum(top_right);\n    if (mode == PorkchopMode::PIGSYNC_DEVICE_SELECT) {\n        char chBuf[12];\n        uint8_t ch = PigSyncMode::getDataChannel();\n        snprintf(chBuf, sizeof(chBuf), "CH:%02d", ch);\n        bottomBar.drawString(chBuf, DISPLAY_W - 2, 3);\n    } else if (mode == PorkchopMode::MENU ||\n''',
    '''    // Right side: W33Z status or uptime.\n    bottomBar.setTextDatum(top_right);\n    if (mode == PorkchopMode::PIGSYNC_DEVICE_SELECT) {\n        bottomBar.drawString(StellaLink::isRegistered() ? "ONLINE" : (StellaLink::isPaired() ? "PAIRED" : "USB"), DISPLAY_W - 2, 3);\n    } else if (mode == PorkchopMode::MENU ||\n''',
    1,
)
display.write_text(text)

print("W33Z USB runtime fix applied.")
print("USB bootstrap is now serviced at the top of loop().")
print("Legacy PigSync display rendering is removed from W33Z Sync.")
print("Next: pio run -e m5cardputer -t clean && pio run -e m5cardputer")
