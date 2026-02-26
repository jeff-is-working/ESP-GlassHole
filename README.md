# ESP-GlassHole

[![Build & Release Firmware](https://github.com/jeff-is-working/ESP-GlassHole/actions/workflows/release.yml/badge.svg)](https://github.com/jeff-is-working/ESP-GlassHole/actions/workflows/release.yml)
[![License: AGPL-3.0](https://img.shields.io/badge/License-AGPL--3.0-blue.svg)](LICENSE)
[![GitHub Release](https://img.shields.io/github/v/release/jeff-is-working/ESP-GlassHole)](https://github.com/jeff-is-working/ESP-GlassHole/releases/latest)

ESP32 firmware that passively detects AR/smart glasses via Bluetooth Low Energy and flashes the onboard LED to alert you. A "glasshole detector" for the age of Meta Ray-Bans and Snap Spectacles.

> **WARNING:** Harassing someone because you suspect they are wearing a recording device can be a criminal offence. **Do not harass anyone.** False positives are inherent in BLE-based detection. A detection alert does not prove someone is recording you.

## How It Works

Smart glasses broadcast BLE advertisement packets containing a **manufacturer-specific data** field. The first two bytes identify the company (assigned by the Bluetooth SIG). This company ID is mandatory per the Bluetooth specification and cannot be randomized, unlike MAC addresses (which Meta Ray-Bans do randomize).

The ESP32 runs continuous BLE scans, parses every advertisement, and checks it against a database of known smart glasses manufacturers. When a match is found above the RSSI threshold, the onboard LED blinks at a rate proportional to proximity and a JSON detection event is emitted over USB serial.

### Detection Methods

| Priority | Method | Reliability | Notes |
|----------|--------|-------------|-------|
| 1 | BLE Company ID | High | Mandatory per BT spec, immutable |
| 2 | BLE Service UUID | High | Meta `0xFD5F`, Google Eddystone |
| 3 | Device name pattern | Medium | 16 patterns: "rayban", "spectacles", "vuzix", etc. |
| 4 | Manufacturer data fingerprint | High | `META_RB_GLASS` byte sequence |
| 5 | MAC OUI prefix | Low | 5 known Meta/Luxottica prefixes (BLE MACs can be random) |

### LED Behavior

| LED Pattern | RSSI | Estimated Distance |
|-------------|------|--------------------|
| Off / dim blue | Below threshold | Nothing detected |
| Slow blink (1 Hz) | >= -75 dBm | Far (~10-15 m) |
| Fast blink (4 Hz) | >= -65 dBm | Medium (~3-10 m) |
| Rapid strobe (10 Hz) | >= -55 dBm | Very close (< 3 m) |

On boards with RGB LEDs (ESP32-S3, ESP32-C3): red = alert, dim blue = scanning idle.

## Detected Devices

**25 manufacturers** across three confidence tiers:

### High Confidence (always on)

| Company | Product | Company ID | Camera |
|---------|---------|------------|--------|
| Meta Platforms | Ray-Ban Smart Glasses | `0x01AB`, `0x058E` | Yes |
| Luxottica Group | Ray-Ban / Oakley Meta | `0x0D53` | Yes |
| Snap Inc | Spectacles | `0x03C2` | Yes |
| Vuzix Corporation | Blade / Shield / M400 | `0x060C` | Yes |

### Medium Confidence (on by default)

| Company | Product | Company ID | Camera |
|---------|---------|------------|--------|
| Google | Glass / Glass EE2 | `0x00E0`, `0x018E` | Yes |
| TCL Communication | RayNeo X2 / Air | `0x0BC6` | Yes |
| Sony Corporation | SmartEyeglass | `0x012D` | Yes |
| Seiko Epson | Moverio BT Series | `0x0040` | Yes |
| Meizu Technology | MYVU AR Glasses | `0x03AB` | Yes |
| Kopin Corporation | Solos AirGo | `0x041F` | No |
| Thalmic Labs (North) | North Focals | `0x0562` | No |

### Low Confidence (off by default)

These company IDs are shared across all products from large manufacturers. Enabling this tier will trigger on phones, headphones, laptops, and other non-glasses devices.

Apple, Xiaomi, Oppo, Huawei, Samsung, Amazon, Bose, Lenovo, HTC, ByteDance, Razer, Fauna

Enable in `firmware/include/config.h`:
```c
#define ENABLE_TIER_LOW true
```

## Hardware

Works on any ESP32 board with BLE. No additional wiring or components required.

| Board | Notes |
|-------|-------|
| **ESP32 DevKit** | Recommended. Cheap, widely available. BLE 4.x. |
| **ESP32-S3 DevKitC** | RGB LED, BLE 5.x, USB-C |
| **ESP32-C3 DevKitM** | Compact, RGB LED, RISC-V |
| **Seeed XIAO ESP32-S3** | Tiny form factor, USB-C |

## Quick Start

### Option 1: Pre-compiled Firmware (easiest)

Download a pre-built `.bin` from the [latest release](https://github.com/jeff-is-working/ESP-GlassHole/releases/latest) and flash it:

```bash
pip install esptool

# ESP32 DevKit
esptool.py --chip esp32 --port /dev/ttyUSB0 write_flash 0x0 ESP-GlassHole-esp32dev-v1.0.0.bin

# ESP32-S3
esptool.py --chip esp32s3 --port /dev/ttyACM0 write_flash 0x0 ESP-GlassHole-esp32-s3-v1.0.0.bin
```

On macOS, the port is typically `/dev/cu.usbserial-*` or `/dev/cu.usbmodem*`.

### Option 2: Build from Source

Requires [PlatformIO](https://platformio.org/install) (CLI or VS Code extension).

```bash
git clone https://github.com/jeff-is-working/ESP-GlassHole.git
cd ESP-GlassHole/firmware

# Build for ESP32 DevKit (default)
pio run

# Build for a specific board
pio run -e esp32-s3

# Flash (connect board via USB first)
pio run -e esp32dev -t upload

# Monitor serial output (115200 baud)
pio device monitor
```

## Serial Protocol

JSON lines over USB at 115200 baud. Pipe to `jq` for readable output:

```bash
pio device monitor | jq .
```

### Message Types

**Boot** (on startup):
```json
{"type":"boot","board":"ESP32","version":"1.0.0"}
```

**Detection** (glasses found):
```json
{
  "type": "detection",
  "mac": "7c:2a:9e:xx:xx:xx",
  "company": "Meta Platforms",
  "product": "Ray-Ban Meta",
  "reason": "Company ID 0x01AB (Meta Platforms)",
  "rssi": -62,
  "hasCamera": true,
  "tier": 0,
  "companyId": "0x01AB",
  "ts": 12345
}
```

**Status** (periodic, every 10s):
```json
{
  "type": "status",
  "board": "ESP32",
  "uptime": 60,
  "freeHeap": 145000,
  "totalScans": 12,
  "totalDetections": 3,
  "trackedDevices": 2,
  "alertActive": true
}
```

**Heartbeat** (every 30s):
```json
{"type":"heartbeat","uptime":90,"freeHeap":144800}
```

## Configuration

All settings are compile-time constants in [`firmware/include/config.h`](firmware/include/config.h):

| Setting | Default | Description |
|---------|---------|-------------|
| `RSSI_THRESHOLD_DEFAULT` | -75 dBm | Minimum signal strength to trigger alert |
| `ENABLE_TIER_HIGH` | `true` | Meta, Snap, Vuzix, Luxottica |
| `ENABLE_TIER_MEDIUM` | `true` | Google, TCL, Sony, Epson, etc. |
| `ENABLE_TIER_LOW` | `false` | Apple, Samsung, Xiaomi, etc. (high FP risk) |
| `LED_ALERT_DURATION_MS` | 5000 | How long LED flashes per detection event |
| `DETECTION_COOLDOWN_MS` | 10000 | Suppress re-alerts for same device within window |
| `BLE_SCAN_TIME` | 5 | BLE scan duration per cycle (seconds) |
| `MAX_TRACKED_DEVICES` | 32 | Maximum simultaneous tracked devices |

## Limitations

- **Detection requires BLE advertising.** Glasses that are connected to a paired phone may stop advertising and become invisible to scanning.
- **False positives from shared company IDs.** VR headsets (Quest), phones, and other products from the same manufacturer share the same BLE company ID.
- **RSSI distance is approximate.** Walls, bodies, and multipath reflections significantly affect signal strength. Indoor distances are roughly half of open-space estimates.
- **LOW tier is off by default** because Apple/Samsung/Xiaomi company IDs trigger on every phone, laptop, and headphone nearby.

## Project Structure

```
firmware/                       ESP32 firmware (PlatformIO)
  src/main.cpp                  Detection engine, LED control, serial output
  include/
    glasses_database.h          Detection database: company IDs, OUIs, UUIDs, name patterns
    config.h                    Compile-time settings: RSSI, tiers, timing
  platformio.ini                Multi-board build configuration
.github/workflows/
  release.yml                   CI: build firmware for all boards on tagged release
app/                            Original Android app (upstream, not actively developed)
```

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines on adding new device signatures, submitting pull requests, and reporting issues.

To add a new glasses manufacturer, edit [`firmware/include/glasses_database.h`](firmware/include/glasses_database.h) and add an entry to the appropriate array:

- **BLE Company ID**: `GLASSES_COMPANY_IDS[]` — look up the ID in the [Bluetooth SIG Assigned Numbers](https://www.bluetooth.com/specifications/assigned-numbers/) registry
- **Device name pattern**: `GLASSES_NAME_PATTERNS[]` — case-insensitive substring match
- **MAC OUI prefix**: `GLASSES_OUI_PREFIXES[]` — from IEEE MA-L registry or field captures
- **Service UUID**: `GLASSES_SERVICE_UUIDS[]` — 16-bit BLE service UUIDs

## Related Projects

- [yj_nearbyglasses](https://github.com/yjeanrenaud/yj_nearbyglasses) — Android app that inspired this project (upstream fork)
- [glass-detect](https://github.com/sh4d0wm45k/glass-detect) — ESP32 "GLASSHOLE" LED sign
- [banrays](https://github.com/NullPxl/banrays) — Wearable glasses that detect smart glasses
- [ouispy-detector](https://github.com/colonelpanichacks/ouispy-detector) — Multi-target BLE scanner
- [oui-spy-too](https://github.com/jeff-is-working/oui-spy-too) — ESP32+Pi surveillance tech scanner (sister project)

## Legal

This is an educational and security research tool. It performs passive, receive-only BLE scanning. It does not transmit, interfere with, or jam any wireless device. Check local laws before use.

**Do not harass anyone.** False positives are inherent in BLE-based detection. A detection alert does not prove someone is recording you.

## License

[AGPL-3.0](LICENSE) (inherited from [upstream](https://github.com/yjeanrenaud/yj_nearbyglasses))
