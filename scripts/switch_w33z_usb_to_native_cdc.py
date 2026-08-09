#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ini = ROOT / "platformio.ini"
text = ini.read_text()
old = "    -DARDUINO_USB_MODE=1\n    -DARDUINO_USB_CDC_ON_BOOT=1\n"
new = "    -DARDUINO_USB_MODE=0\n    -DARDUINO_USB_CDC_ON_BOOT=1\n"
if old not in text:
    raise SystemExit("Expected Hardware CDC/JTAG flags not found in platformio.ini")
text = text.replace(old, new, 1)
ini.write_text(text)

print("Switched ESP32-S3 USB from Hardware CDC/JTAG to native USB CDC (TinyUSB).")
print("This keeps USB CDC on boot, so Serial remains the application serial port,")
print("but host->device traffic is handled by the native USB CDC stack.")
print("Next: pio run -e m5cardputer -t clean && pio run -e m5cardputer")
