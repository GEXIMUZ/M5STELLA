#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected exactly 1 match, found {count}")
    return text.replace(old, new, 1)

# Expose a dedicated fast USB service tick. This keeps USB bootstrap independent
# of Wi-Fi/TLS/telemetry scheduling and lets main.cpp service it at the very top
# of every loop iteration.
h = ROOT / "src/stella/w33z_link.h"
text = h.read_text()
needle = "void update();\n"
replacement = "void update();\n\n// Fast USB bootstrap service. Safe to call every loop before other workload.\nvoid serviceUsb();\n"
text = replace_once(text, needle, replacement, "w33z_link.h serviceUsb")
h.write_text(text)

cpp = ROOT / "src/stella/w33z_link.cpp"
text = cpp.read_text()

# The USB-first integration script defines serviceUsbBootstrap() in the private
# namespace. Expose a public wrapper and stop relying exclusively on update().
needle = '''void update() {
    serviceUsbBootstrap();
'''
replacement = '''void serviceUsb() {
    serviceUsbBootstrap();
}

void update() {
    serviceUsbBootstrap();
'''
text = replace_once(text, needle, replacement, "w33z_link.cpp public USB service")
cpp.write_text(text)

main = ROOT / "src/main.cpp"
text = main.read_text()
needle = '''void loop() {
    M5Cardputer.update();
'''
replacement = '''void loop() {
    // USB enrollment must stay responsive even when Wi-Fi/recon/UI work is busy.
    // Service CDC input before touching the rest of the firmware loop.
    StellaLink::serviceUsb();

    M5Cardputer.update();
'''
text = replace_once(text, needle, replacement, "main.cpp early USB service")
main.write_text(text)

print("Applied high-priority W33Z USB bootstrap servicing.")
print("Changed:")
print("  src/stella/w33z_link.h")
print("  src/stella/w33z_link.cpp")
print("  src/main.cpp")
