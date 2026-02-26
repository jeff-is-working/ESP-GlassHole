# Contributing to ESP-GlassHole

Thank you for your interest in improving smart glasses detection. Contributions are welcome via pull requests and issues.

## Adding New Device Signatures

The most impactful contribution is expanding the detection database. If you have a pair of smart glasses (or encounter them in the wild), you can capture their BLE advertisements and submit the data.

### What to capture

Use a BLE scanner app (nRF Connect, LightBlue, or the ESP-GlassHole serial output itself) to record:

1. **BLE Company ID** — first 2 bytes of manufacturer-specific data (AD Type `0xFF`), little-endian
2. **Device name** — the BLE local name string
3. **Service UUIDs** — any 16-bit or 128-bit UUIDs advertised
4. **MAC OUI prefix** — first 3 bytes of the MAC address (note: may be randomized)
5. **Raw manufacturer data** — full hex dump for fingerprinting

### Where to add it

Edit [`firmware/include/glasses_database.h`](firmware/include/glasses_database.h):

| Data type | Array | Example |
|-----------|-------|---------|
| Company ID | `GLASSES_COMPANY_IDS[]` | `{ 0x01AB, "Meta Platforms", "Ray-Ban Meta", true, TIER_HIGH }` |
| Name pattern | `GLASSES_NAME_PATTERNS[]` | `{ "rayban", "Meta Ray-Ban", true }` |
| OUI prefix | `GLASSES_OUI_PREFIXES[]` | `{ { 0x7C, 0x2A, 0x9E }, "Meta Platforms Technologies" }` |
| Service UUID | `GLASSES_SERVICE_UUIDS[]` | `{ 0xFD5F, "Meta Platforms", "Meta BLE Service" }` |
| Mfg data fingerprint | `GLASSES_MFG_DATA_PATTERNS[]` | `{ 0x058E, "4D455441...", "Meta Ray-Ban" }` |

### Choosing the right tier

| Tier | Criteria | Example |
|------|----------|---------|
| `TIER_HIGH` | Dedicated glasses/eyewear company. Low false-positive risk. | Meta Platforms Technologies, Vuzix, Snap |
| `TIER_MEDIUM` | Company known to make glasses, but the company ID may also appear on non-glasses products. | Google (Glass vs. Pixel), Sony (SmartEyeglass vs. headphones) |
| `TIER_LOW` | Large ecosystem where glasses are one of hundreds of BLE products. Very high false-positive risk. | Apple, Samsung, Xiaomi |

## Pull Request Guidelines

1. **One device per PR** (or a batch of related devices from the same manufacturer)
2. **Include your source** — link to the Bluetooth SIG entry, a BLE capture log, or the product's FCC filing
3. **Test if possible** — build and flash the firmware, verify detection works
4. **Keep the database sorted** — entries within each tier should be in alphabetical order by company name

## Building and Testing

```bash
cd firmware

# Build for all boards
pio run

# Build and flash a specific board
pio run -e esp32dev -t upload

# Monitor serial output
pio device monitor
```

## Reporting Issues

When filing an issue, include:

- **Board**: ESP32 variant and board name
- **Firmware version**: from the `boot` JSON message
- **Serial output**: relevant JSON lines showing the problem
- **Expected vs. actual behavior**

## Code Style

- C++ (Arduino framework), compiled with PlatformIO
- No dynamic memory allocation in the hot path (scan callbacks)
- JSON output via ArduinoJson
- All detection data is compile-time `static const` arrays

## License

By contributing, you agree that your contributions will be licensed under [AGPL-3.0](LICENSE).
