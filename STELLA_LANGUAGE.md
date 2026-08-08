# Stella language guide

This document is the canonical naming source for the Porkchop -> Stella conversion. The goal is **funny on the surface, precise underneath**. Dog jokes must never hide what a wireless function actually does.

## Core rule

Every feature has three names:

1. **Technical identifier** - stable, boring, suitable for protocols and logs.
2. **Stella label** - short dog-themed UI name.
3. **Plain description** - tells the operator exactly what the feature does.

Example:

`scan.passive` -> **SNIFF** -> Passive Wi-Fi reconnaissance.

## Canonical operating language

| Technical meaning | Stella label | UI description |
| --- | --- | --- |
| Idle | **SIT** | Stella is idle and ready. |
| Passive Wi-Fi recon | **SNIFF** | Observe nearby Wi-Fi without active disruption. |
| Wardriving | **PATROL** | GPS-assisted Wi-Fi survey / wardrive. |
| Active authorized capture mode | **BITE** | Active assessment/capture tooling for an authorized target. |
| Pause work | **STAY** | Pause current work. |
| Stop and return idle | **HEEL** | Stop current operation and return to idle. |
| Pull data to W33Z | **FETCH** | Fetch sessions, captures and logs into W33Z. |
| Push config to Stella | **DELIVER** | Deliver profiles/configuration from W33Z. |
| Identify device | **BARK** | Make this Stella identify herself. |
| Broadcast/beacon feature | **HOWL** | Broadcast a configured wireless beacon/profile. |
| Device-to-device sync | **PACK LINK** | Stella-to-Stella local synchronization. |
| Trusted/excluded networks | **FRIENDLY PACK** | Networks excluded from active assessment. |
| Device/statistics screen | **WARDOG STATS** | Lifetime/session statistics and progression. |
| BLE tools | **BLUE PAWS** | Bluetooth Low Energy utilities. |
| Charging / low power | **NAP** | Charging or low-power state. |
| Warning | **GROWL** | Attention required. |
| Error | **HOWL!** | Operation failed or device needs attention. |

## Porkchop migration map

The legacy code names are not all renamed immediately because they are deeply coupled to storage paths, config enums and mode switching. User-facing copy should migrate first; internal symbols follow after compatibility shims are proven.

| Legacy Porkchop term | Stella target | Notes |
| --- | --- | --- |
| `PORKCHOP` | `STELLA` | Product/device identity. |
| `Piglet` | `Stella` / `Wardog` | Personality/avatar identity. |
| `OINK` | `BITE` | Legacy OINK combines capture and active behavior; UI must say what is active. |
| `DO NO HAM` | `SNIFF` | Passive reconnaissance. |
| `WARHOG` | `PATROL` | Wardriving / GPS survey. |
| `PIGSYNC` | `PACK LINK` | Device-to-device synchronization. |
| `PIGGY BLUES` | `BLUE PAWS` | BLE feature family. |
| `BACON` | `HOWL` | Beacon/broadcast feature. |
| `BOAR BROS` | `FRIENDLY PACK` | Exclusion/trusted network list. |
| `SWINE STATS` | `WARDOG STATS` | Statistics/progression screen. |
| `OINK OINK` boot copy | `WOOF WOOF` | Boot identity. |
| pig grunt/oink SFX | bark/woof/howl family | No pig vocalizations remain in final Stella sound bank. |

## Personality / animation direction

Stella is a fox Pomeranian. Personality should feel lively and mischievous without turning technical status into nonsense.

Recommended visual metaphors:

- scanning: nose down, ears moving, sniff particles / signal lines
- wardriving: walking/patrol animation, little GPS trail
- network discovered: ears perk up
- interesting target tagged: paw marker / territory flag
- location marker: Stella marks the spot; a tree/lamppost gag can be used sparingly
- sync/fetch: Stella returns carrying a data packet
- upload/deliver: Stella drops a packet into a terminal/mailbox
- idle: sitting, tail movement, occasional look-around
- warning: small growl / raised ears
- error: confused head tilt or short howl
- charging: curled-up nap with charging icon
- GPS acquisition: tracking/sniffing the ground/sky
- no GPS: confused compass/head tilt

The old pig/tree gag should become a dog-specific equivalent: **Stella marking a tree/lamppost**, not a literal one-for-one animal swap everywhere.

## Audio direction

No final Stella build should contain an intentional pig vocalization.

Audio families:

- **woof** - normal confirmation / device identify
- **double woof** - boot/personality moment
- **short bark** - discovery/event attention
- **growl** - warning
- **howl** - major error / dramatic event
- **happy yip** - achievement/level-up
- **sniff ticks** - subtle scan feedback; should not fire constantly enough to annoy the operator

Frequent operational sounds remain restrained. Stella is a tool first, mascot second.

## W33Z protocol language

Do not put dog jokes into machine endpoints. Keep:

- `scan.start`
- `scan.pause`
- `scan.stop`
- `sync.pull`
- `sync.push`
- `device.identify`
- `display.snapshot`
- `display.input`
- `logs.tail`
- `firmware.prepare`

W33Z can render those as **Sniff / Stay / Heel / Fetch / Deliver / Bark / Look / Paw Control / Track / Groom**.

## Security wording

Active functionality must always be visually distinguishable from passive survey functionality. `SNIFF` and `PATROL` are passive. `BITE` is active and should show an explicit authorized-assessment indicator before transmitting disruptive frames.
