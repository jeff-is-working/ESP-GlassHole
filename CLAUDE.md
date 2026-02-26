# CLAUDE.md — ESP-GlassHole

## Project Overview

ESP32 BLE scanner that detects AR/smart glasses nearby and flashes the LED as a visual alert. Forked from [yj_nearbyglasses](https://github.com/yjeanrenaud/yj_nearbyglasses) (Android app) and reimplemented as standalone ESP32 firmware.

**Author:** Jeff Records / Circle 6 Systems
**Repo:** [jeff-is-working/ESP-GlassHole](https://github.com/jeff-is-working/ESP-GlassHole)
**Upstream:** [yjeanrenaud/yj_nearbyglasses](https://github.com/yjeanrenaud/yj_nearbyglasses)
**License:** AGPL-3.0 (inherited from upstream)

## Architecture

```
ESP32 BLE Scan → Parse advertisements → Match detection database
                                      → RSSI filter
                                      → LED alert (blink rate = proximity)
                                      → Serial JSON output
```

Completely passive — receive-only BLE scanning, no transmission.

## Repository Layout

```
firmware/                        ESP32 firmware (PlatformIO)
  src/main.cpp                   Main: BLE scan, detection engine, LED control, JSON output
  include/
    glasses_database.h           Detection database: company IDs, OUIs, names, UUIDs
    config.h                     Tunable settings: RSSI, tiers, timing, LED
  platformio.ini                 Multi-board build config

app/                             Original Android app (upstream, not actively developed)
```

## Detection Methods (priority order)

1. **BLE Company ID** (primary) — mandatory in BT spec, cannot be randomized
2. **BLE Service UUID** — Meta 0xFD5F, Google Eddystone
3. **Device name patterns** — "rayban", "spectacles", "vuzix", etc.
4. **MAC OUI prefix** — supplementary heuristic (BLE MACs can be random)
5. **Manufacturer data fingerprint** — "META_RB_GLASS" byte sequence

## Detection Tiers

| Tier | Default | FP Risk | Devices |
|------|---------|---------|---------|
| HIGH | ON | Low | Meta, Luxottica, Snap, Vuzix |
| MEDIUM | ON | Moderate | Google, TCL/RayNeo, Meizu, Sony, Epson, North/Thalmic, Kopin |
| LOW | OFF | High | Apple, Xiaomi, Oppo, Huawei, Samsung, Amazon, Bose, Lenovo, HTC, ByteDance, Razer, Fauna |

Toggle tiers in `config.h`: `ENABLE_TIER_HIGH`, `ENABLE_TIER_MEDIUM`, `ENABLE_TIER_LOW`.

## LED Behavior

| RSSI | Distance | Blink Rate |
|------|----------|------------|
| >= -55 dBm | Very close (<3m) | 10 Hz strobe |
| >= -65 dBm | Medium (3-10m) | 4 Hz fast blink |
| >= -75 dBm | Far (10-15m) | 1 Hz slow blink |
| < -75 dBm | Below threshold | No alert |

RGB LED boards (ESP32-S3, C3): Red = alert, dim blue = scanning idle.

## Building

```bash
cd firmware
pio run -e esp32dev        # Build for ESP32 DevKit
pio run -e esp32-s3        # Build for ESP32-S3
pio run -e xiao-s3         # Build for Seeed XIAO ESP32-S3
pio run -e esp32dev -t upload   # Flash
pio device monitor              # Serial monitor (115200 baud)
```

PlatformIO path on this Mac: `/Users/jeffrecords/.platformio/penv/bin/pio`

## Serial Protocol (USB, 115200 baud)

```json
{"type":"boot","board":"ESP32","version":"1.0.0"}
{"type":"detection","mac":"7c:2a:9e:xx:xx:xx","company":"Meta Platforms","product":"Ray-Ban Meta","reason":"Company ID 0x01AB (Meta Platforms)","rssi":-62,"hasCamera":true,"tier":0,"companyId":"0x01AB","ts":12345}
{"type":"status","board":"ESP32","uptime":60,"freeHeap":145000,"totalScans":12,"totalDetections":3,"trackedDevices":2,"alertActive":true}
{"type":"heartbeat","uptime":90,"freeHeap":144800}
```

## Key Files to Edit

- **Add new glasses manufacturer:** `glasses_database.h` → `GLASSES_COMPANY_IDS[]`
- **Add new name pattern:** `glasses_database.h` → `GLASSES_NAME_PATTERNS[]`
- **Change RSSI / timing:** `config.h`
- **Change LED behavior:** `main.cpp` → `updateLED()`, `getBlinkRate()`

## Research Sources

- [yj_nearbyglasses](https://github.com/yjeanrenaud/yj_nearbyglasses) — BLE company ID detection (Android)
- [glass-detect](https://github.com/sh4d0wm45k/glass-detect) — ESP32 "GLASSHOLE" LED, OUI-based
- [banrays](https://github.com/NullPxl/banrays) — wearable glasses detector, found 0xFD5F + META_RB_GLASS
- [ouispy-detector](https://github.com/colonelpanichacks/ouispy-detector) — multi-target BLE OUI scanner
- [NordicSemiconductor/bluetooth-numbers-database](https://github.com/NordicSemiconductor/bluetooth-numbers-database) — BLE company ID registry
- [Bluetooth SIG Assigned Numbers](https://www.bluetooth.com/specifications/assigned-numbers/)

## Gotchas

- BLE MAC addresses ARE randomized on Meta Ray-Bans — OUI matching is unreliable alone
- Detection only works when glasses are advertising (power-on, pairing, case removal) — once connected to a phone, they go silent
- Company ID matching produces false positives from VR headsets (Quest) and other products
- LOW tier is off by default because Apple/Samsung/Xiaomi IDs would trigger on phones constantly
- ESP32 (original) only supports BLE 4.x Legacy Advertising — sufficient for glasses
- `huge_app.csv` partition table required due to BLE library size
