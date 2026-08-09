#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
cpp = ROOT / "src/stella/w33z_link.cpp"
text = cpp.read_text()
old = '''void update() {
    serviceUsbBootstrap();
    if (!cfg.enabled) return;
    if (!ensureWifi()) return;
'''
new = '''void update() {
    // USB enrollment must stay available before any network/TLS activity.
    // Until a bearer token exists, Stella is intentionally USB-bootstrap-only.
    serviceUsbBootstrap();
    if (!cfg.enabled) return;
    if (bearerToken.isEmpty()) return;
    if (!ensureWifi()) return;
'''
count = text.count(old)
if count != 1:
    raise SystemExit(f"expected exactly 1 StellaLink::update() preamble, found {count}")
cpp.write_text(text.replace(old, new, 1))
print("Patched StellaLink: no Wi-Fi/TLS attempts before USB enrollment token exists.")
print("Next: pio run -e m5cardputer")
