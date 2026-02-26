# ESP-GlassHole

ESP32 firmware that detects AR/smart glasses nearby via BLE and flashes the LED to alert you. A "glasshole detector" for the age of Meta Ray-Bans and Snap Spectacles.

> **WARNING:** HARASSING someone because you think they are wearing a covert surveillance device can be a criminal offence. **DO NOT HARASS ANYONE.** False positives are likely. Think before you act.

## What It Does

Passively scans Bluetooth Low Energy (BLE) advertisements for known smart glasses and AR headsets. When detected, the LED blinks — faster means closer.

| LED Pattern | Meaning |
|-------------|---------|
| Off / dim blue | Scanning, nothing detected |
| Slow blink (1 Hz) | Glasses detected, far away (~10-15m) |
| Fast blink (4 Hz) | Glasses detected, medium range (~3-10m) |
| Rapid strobe (10 Hz) | Glasses very close (<3m) |

## Detected Devices

**25 manufacturers** across 3 detection tiers:

### Always On (High Confidence)
- **Meta** Ray-Ban Smart Glasses (0x01AB, 0x058E)
- **Luxottica** Ray-Ban / Oakley frames (0x0D53)
- **Snap** Spectacles (0x03C2)
- **Vuzix** Blade, Shield, M400 (0x060C)

### On by Default (Medium Confidence)
- **Google** Glass (0x00E0, 0x018E)
- **TCL** RayNeo (0x0BC6)
- **Sony** SmartEyeglass (0x012D)
- **Epson** Moverio (0x0040)
- **Meizu** MYVU (0x03AB)
- **Kopin/Solos** AirGo (0x041F)
- **North** Focals (0x0562)

### Off by Default (High False-Positive Risk)
Apple Vision Pro, Xiaomi AI Glasses, Oppo Air Glass, Huawei Eyewear, Samsung, Amazon Echo Frames, Bose Frames, Lenovo ThinkReality, HTC Vive, ByteDance Pico, Razer Anzu, Fauna

Enable in `firmware/include/config.h` by setting `ENABLE_TIER_LOW` to `true`.

## Detection Methods

1. **BLE Company ID** — primary method, mandatory per Bluetooth spec, immutable
2. **BLE Service UUID** — Meta (0xFD5F), Google Eddystone
3. **Device name matching** — "rayban", "spectacles", "vuzix", etc. (16 patterns)
4. **MAC OUI prefix** — 5 known Meta/Luxottica prefixes (supplementary)
5. **Manufacturer data fingerprint** — META_RB_GLASS byte sequence

## Hardware

Works on any ESP32 board with BLE:
- **ESP32 DevKit** (recommended — cheap, widely available)
- **ESP32-S3** (RGB LED support, BLE 5.x for future expansion)
- **ESP32-C3** (compact, RGB LED)
- **Seeed XIAO ESP32-S3** (tiny form factor)

Only needs the ESP32 board itself — no additional wiring or components. Uses the onboard LED.

## Quick Start

### Prerequisites
- [PlatformIO](https://platformio.org/install) (CLI or VS Code extension)
- USB cable
- Any ESP32 board

### Build & Flash

```bash
git clone https://github.com/jeff-is-working/ESP-GlassHole.git
cd ESP-GlassHole/firmware

# Build
pio run -e esp32dev

# Flash (connect ESP32 via USB first)
pio run -e esp32dev -t upload

# Monitor serial output
pio device monitor
```

For ESP32-S3: replace `esp32dev` with `esp32-s3`

### Serial Output

JSON lines at 115200 baud:

```json
{"type":"detection","mac":"7c:2a:9e:xx:xx:xx","company":"Meta Platforms","product":"Ray-Ban Meta","reason":"Company ID 0x01AB","rssi":-62,"hasCamera":true,"tier":0}
{"type":"status","board":"ESP32","uptime":60,"totalScans":12,"totalDetections":3}
```

## Configuration

Edit `firmware/include/config.h`:

| Setting | Default | Description |
|---------|---------|-------------|
| `RSSI_THRESHOLD_DEFAULT` | -75 dBm | Minimum signal strength to trigger |
| `ENABLE_TIER_HIGH` | true | Meta, Snap, Vuzix, Luxottica |
| `ENABLE_TIER_MEDIUM` | true | Google, TCL, Sony, Epson |
| `ENABLE_TIER_LOW` | false | Apple, Samsung, Xiaomi, etc. |
| `LED_ALERT_DURATION_MS` | 5000 | How long LED flashes per detection |
| `DETECTION_COOLDOWN_MS` | 10000 | Deduplicate same device for 10s |

## How It Works

Smart glasses broadcast BLE advertisement packets containing a **manufacturer-specific data** field. The first 2 bytes identify the company (assigned by Bluetooth SIG). This company ID is mandatory and cannot be randomized — unlike MAC addresses, which Meta Ray-Bans do randomize.

The ESP32 runs continuous BLE scans, parses every advertisement, and checks against a database of known glasses manufacturers. When a match is found above the RSSI threshold, it triggers the LED and emits a JSON detection event.

### Limitations

- **Detection only works during BLE advertising** — glasses that are connected to a paired phone may stop advertising and become invisible
- **False positives** from VR headsets (Quest), phones, and other products sharing the same manufacturer company ID
- **BLE range** varies significantly based on environment — RSSI distance estimates are approximate
- **LOW tier disabled by default** because Apple/Samsung/Xiaomi IDs would trigger on every phone nearby

## Forked From

[yj_nearbyglasses](https://github.com/yjeanrenaud/yj_nearbyglasses) by Yves Jeanrenaud — an Android app with the same purpose. This project reimplements the detection logic as standalone ESP32 firmware with an expanded device database.

## Related Projects

- [glass-detect](https://github.com/sh4d0wm45k/glass-detect) — ESP32 "GLASSHOLE" LED sign
- [banrays](https://github.com/NullPxl/banrays) — wearable glasses that detect smart glasses
- [oui-spy-too](https://github.com/jeff-is-working/oui-spy-too) — ESP32+Pi surveillance tech scanner (sister project)
- [ouispy-detector](https://github.com/colonelpanichacks/ouispy-detector) — multi-target BLE scanner

## Legal

Educational and security research tool. Passive receive-only BLE scanning — does not transmit, interfere with, or jam any device. Check local laws before use.

**Do not harass anyone.** False positives are inherent in this approach. A detection alert does not prove someone is recording you.

## License

[AGPL-3.0](LICENSE) (inherited from upstream)
