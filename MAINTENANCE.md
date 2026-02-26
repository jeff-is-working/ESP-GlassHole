# Maintenance

This project is actively maintained by [Jeff Records](https://github.com/jeff-is-working) / Circle 6 Systems.

## Release Process

1. Update the version string in `firmware/src/main.cpp` (search for `"version"`)
2. Commit and push to `main`
3. Tag the release: `git tag v1.x.x && git push origin v1.x.x`
4. GitHub Actions automatically builds firmware for all supported boards and creates a release with downloadable `.bin` files

## Keeping the Detection Database Current

Smart glasses manufacturers change BLE advertisement formats, add new products, and register new Bluetooth SIG company IDs. To keep the database accurate:

- Monitor the [Bluetooth SIG Assigned Numbers](https://www.bluetooth.com/specifications/assigned-numbers/) registry for new company ID assignments
- Check product teardowns and FCC filings for BLE advertisement details
- Field-test with actual devices when possible
- Review community reports in GitHub Issues

## Supported Boards

Firmware is built and tested for:

| Board | PlatformIO Environment | Status |
|-------|----------------------|--------|
| ESP32 DevKit (generic) | `esp32dev` | Primary target, actively tested |
| ESP32-S3 DevKitC-1 | `esp32-s3` | Supported |
| ESP32-C3 DevKitM-1 | `esp32-c3` | Supported |
| Seeed XIAO ESP32-S3 | `xiao-s3` | Supported |
