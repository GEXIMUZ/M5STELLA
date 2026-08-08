# M5STELLA architecture

M5STELLA is a staged evolution of M5PORKCHOP into **Stella the Wardog**, an M5Cardputer wireless field companion designed to integrate directly with W33Z.

The original MIT license and attribution remain intact. The migration is intentionally incremental so existing hardware behavior, SD cards and field data are not destroyed by a giant one-shot rename.

## Design rules

1. **Dog personality outside, boring protocol names inside.**
   User-facing actions can be `Sniff`, `Stay`, `Heel`, `Fetch`, `Deliver` and `Bark`; the protocol remains `scan.start`, `scan.pause`, `scan.stop`, `sync.pull`, `sync.push` and `device.identify`.
2. **Passive remote defaults.**
   Remote `Sniff` starts passive WARHOG wardriving. Active radio actions are never silently triggered by W33Z.
3. **Capability driven.**
   W33Z only unlocks features the firmware explicitly advertises.
4. **Transport independent.**
   Wi-Fi, USB and BLE converge on Stella protocol v1.
5. **Backward compatible during migration.**
   Legacy config paths and internal Porkchop symbols remain temporarily where renaming them would break existing installations. New code must use the Stella layer.
6. **No fake telemetry.**
   Missing GPS, battery, display frames, captures or sync state stay missing/unknown.

## Stella protocol v1

Firmware identity is defined in `src/stella/identity.h`.

Current capabilities:

- telemetry
- gps
- wifi_scan
- capture_inventory
- file_sync
- display_mirror
- display_input
- firmware_update
- device_logs
- ble_provisioning

These are capability declarations, not a promise that every transport is implemented yet. W33Z may present a capability as unavailable until the corresponding firmware path is completed.

## Wi-Fi W33Z link

`src/stella/w33z_link.cpp` is the first real transport implementation.

The link is **opt-in**. Copy `stella_link.example.json` to `/stella_link.json` on the Stella SD card and replace `W33Z-IP` with the LAN IP/hostname of the machine exposing W33Z on port 3033.

Example:

```json
{
  "enabled": true,
  "baseUrl": "http://W33Z-IP:3033",
  "wifiAutoConnect": false,
  "heartbeatMs": 5000,
  "commandPollMs": 1500,
  "reconnectMs": 10000
}
```

`wifiAutoConnect=false` is the conservative default because a STA connection can influence Wi-Fi scanning behavior. When false, Stella only talks to W33Z while Wi-Fi is already connected by another firmware flow. When enabled it reuses the existing Wi-Fi credentials from firmware configuration.

The transport currently implements:

- deterministic device identity from ESP32 eFuse MAC
- W33Z handshake
- periodic telemetry heartbeat
- GPS telemetry when a real fix exists
- radio channel and network-count telemetry
- command polling
- command result acknowledgements

## Command mapping

| W33Z protocol | Stella UI | Current firmware behavior |
| --- | --- | --- |
| `scan.start` | Sniff | Passive WARHOG wardrive |
| `scan.pause` | Stay | Return idle, report paused |
| `scan.stop` | Heel | Return idle |
| `device.identify` | Bark | Temporary synthesized woof |
| `sync.pull` | Fetch | Reserved; not wired yet |
| `sync.push` | Deliver | Reserved; not wired yet |
| `display.snapshot` | Look | Reserved; not wired yet |
| `device.reboot` | Roll Over | Disabled until authenticated pairing |
| `logs.tail` | Track | Reserved; not wired yet |
| `firmware.prepare` | Groom | Reserved; not wired yet |

## Migration phases

### Phase 1 - core identity and protocol

- Stella firmware identity
- W33Z Wi-Fi link
- heartbeat and command loop
- safe remote wardrive control
- version/build rebrand

### Phase 2 - personality replacement

- `piglet` engine becomes Stella personality engine
- pig art replaced with fox Pomeranian art
- pig-specific copy rewritten into dog behavior
- oinks replaced with barks/woofs/howls
- tree-biting/foraging gags replaced with dog equivalents such as sniffing, marking territory and chasing signals

### Phase 3 - device transports

- USB CDC/serial transport
- BLE provisioning and lightweight control
- automatic transport discovery in W33Z
- pairing/authentication and per-device trust

### Phase 4 - remote display and sync

- display snapshot/frame transport
- remote button/input events
- bidirectional wardrive/session sync
- capture inventory
- logs
- firmware update staging

### Phase 5 - cleanup

Only after compatibility migration is proven:

- rename legacy internal `Porkchop*` types
- migrate `/m5porkchop` SD layout to `/m5stella`
- migrate config magic/path with fallback import
- rename remaining PIGSYNC/OINK identifiers
- remove compatibility aliases

This avoids turning the firmware into an unbootable rename bomb while still moving every new feature onto Stella-native architecture.
