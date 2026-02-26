/*
 * ESP-GlassHole — Configuration
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
// BLE Scan Settings
// ============================================================
#define BLE_SCAN_TIME          5       // BLE scan duration in seconds
#define BLE_SCAN_INTERVAL_MS   100     // Scan interval (in 0.625ms units = 160)
#define BLE_SCAN_WINDOW_MS     80      // Scan window (in 0.625ms units = 128)

// ============================================================
// RSSI Thresholds (dBm)
// ============================================================
// BLE RSSI rough distance guide (open space):
//   -50 dBm  ~ 1-2 meters   (very close)
//   -60 dBm  ~ 1-3 meters   (close)
//   -70 dBm  ~ 3-10 meters  (medium)
//   -80 dBm  ~ 10-20 meters (far)
//   -90 dBm  ~ 20-40 meters (very far / noise)
//
// Indoors, distances are roughly halved.
#define RSSI_THRESHOLD_DEFAULT -75     // Minimum RSSI to trigger detection
#define RSSI_CLOSE             -55     // Very close — rapid strobe
#define RSSI_MEDIUM            -65     // Medium distance — fast blink
#define RSSI_FAR               -75     // Far — slow blink

// ============================================================
// Detection Tier Settings
// ============================================================
// Enable/disable detection tiers at compile time.
// HIGH is always on. MEDIUM and LOW can be toggled.
#define ENABLE_TIER_HIGH       true
#define ENABLE_TIER_MEDIUM     true
#define ENABLE_TIER_LOW        false   // Off by default (too many false positives)

// ============================================================
// LED Settings
// ============================================================
#define LED_ALERT_DURATION_MS  5000    // How long LED flashes after detection
#define LED_BLINK_SLOW_MS      500     // Slow blink half-period (1 Hz)
#define LED_BLINK_FAST_MS      125     // Fast blink half-period (4 Hz)
#define LED_BLINK_STROBE_MS    50      // Rapid strobe half-period (10 Hz)

// ============================================================
// Serial Output
// ============================================================
#define SERIAL_BAUD            115200
#define STATUS_INTERVAL_MS     10000   // Status message every 10s
#define HEARTBEAT_INTERVAL_MS  30000   // Heartbeat every 30s

// ============================================================
// Notification Cooldown
// ============================================================
// Don't re-alert for the same device within this window.
// Prevents LED strobe from one device continuously triggering.
#define DETECTION_COOLDOWN_MS  10000   // 10 seconds per device

// ============================================================
// Device Tracking
// ============================================================
#define MAX_TRACKED_DEVICES    32      // Max simultaneous tracked devices

#endif // CONFIG_H
