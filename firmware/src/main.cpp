/*
 * ESP-GlassHole — AR/Smart Glasses Detector
 *
 * Passively scans BLE advertisements for known smart glasses and AR headsets.
 * Flashes the onboard LED when glasses are detected nearby.
 * LED blink rate indicates proximity (faster = closer).
 *
 * Detection methods:
 *   1. BLE Company ID matching (primary — mandatory per BT spec)
 *   2. BLE Service UUID matching (secondary)
 *   3. BLE device name pattern matching (tertiary)
 *   4. MAC OUI prefix matching (supplementary heuristic)
 *   5. Manufacturer data fingerprinting (specific product identification)
 *
 * Based on research from:
 *   - yj_nearbyglasses (Yves Jeanrenaud)
 *   - glass-detect (sh4d0wm45k)
 *   - banrays (NullPxl)
 *   - ouispy-detector (colonelpanichacks)
 *
 * Forked from: github.com/yjeanrenaud/yj_nearbyglasses
 * License: AGPL-3.0
 * Author: Jeff Records
 */

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <ArduinoJson.h>

#include "config.h"
#include "glasses_database.h"

// ============================================================
// Board Detection & Pin Configuration
// ============================================================

#if defined(CONFIG_IDF_TARGET_ESP32S3)
  #define BOARD_TYPE "ESP32-S3"
  #ifdef RGB_BUILTIN
    #define HAS_RGB_LED true
    #define LED_PIN RGB_BUILTIN
  #else
    #define HAS_RGB_LED false
    #define LED_PIN 48
  #endif
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
  #define BOARD_TYPE "ESP32-C3"
  #ifdef RGB_BUILTIN
    #define HAS_RGB_LED true
    #define LED_PIN RGB_BUILTIN
  #else
    #define HAS_RGB_LED false
    #define LED_PIN 8
  #endif
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
  #define BOARD_TYPE "ESP32-C6"
  #ifdef RGB_BUILTIN
    #define HAS_RGB_LED true
    #define LED_PIN RGB_BUILTIN
  #else
    #define HAS_RGB_LED false
    #define LED_PIN 8
  #endif
#else
  #define BOARD_TYPE "ESP32"
  #define HAS_RGB_LED false
  #define LED_PIN 2
#endif

// ============================================================
// Global State
// ============================================================

BLEScan* pBLEScan = nullptr;

// Tracked devices (for cooldown deduplication)
struct TrackedDevice {
    uint8_t  mac[6];
    uint32_t lastSeen;
    int      rssi;
    uint8_t  tier;
    bool     hasCamera;
};

TrackedDevice trackedDevices[MAX_TRACKED_DEVICES];
int trackedDeviceCount = 0;

// LED alert state
volatile bool     alertActive = false;
volatile uint32_t alertStartTime = 0;
volatile int      alertRssi = -100;
volatile bool     alertHasCamera = false;
volatile uint8_t  alertTier = TIER_HIGH;

// Counters
uint32_t totalScans = 0;
uint32_t totalDetections = 0;
uint32_t lastStatusTime = 0;
uint32_t lastHeartbeatTime = 0;

// ============================================================
// LED Control
// ============================================================

void ledOn() {
#if HAS_RGB_LED
    neopixelWrite(LED_PIN, 255, 0, 0);  // Red for alert
#else
    digitalWrite(LED_PIN, HIGH);
#endif
}

void ledOff() {
#if HAS_RGB_LED
    neopixelWrite(LED_PIN, 0, 0, 0);
#else
    digitalWrite(LED_PIN, LOW);
#endif
}

void ledIdle() {
#if HAS_RGB_LED
    neopixelWrite(LED_PIN, 0, 0, 8);  // Dim blue = scanning
#else
    // Standard LED stays off when idle
#endif
}

// Returns blink half-period based on RSSI
uint32_t getBlinkRate(int rssi) {
    if (rssi >= RSSI_CLOSE)  return LED_BLINK_STROBE_MS;
    if (rssi >= RSSI_MEDIUM) return LED_BLINK_FAST_MS;
    return LED_BLINK_SLOW_MS;
}

void updateLED() {
    if (!alertActive) {
        ledIdle();
        return;
    }

    // Check if alert has expired
    if (millis() - alertStartTime > LED_ALERT_DURATION_MS) {
        alertActive = false;
        ledIdle();
        return;
    }

    // Blink pattern based on proximity
    uint32_t blinkRate = getBlinkRate(alertRssi);
    bool on = ((millis() / blinkRate) % 2) == 0;

    if (on) {
        ledOn();
    } else {
        ledOff();
    }
}

void triggerAlert(int rssi, uint8_t tier, bool hasCamera) {
    alertActive = true;
    alertStartTime = millis();
    alertRssi = rssi;
    alertTier = tier;
    alertHasCamera = hasCamera;
}

// ============================================================
// Device Tracking (Cooldown Deduplication)
// ============================================================

// Check if a device was recently seen (within cooldown window)
bool isDeviceCoolingDown(const uint8_t* mac) {
    uint32_t now = millis();
    for (int i = 0; i < trackedDeviceCount; i++) {
        if (memcmp(trackedDevices[i].mac, mac, 6) == 0) {
            if (now - trackedDevices[i].lastSeen < DETECTION_COOLDOWN_MS) {
                // Update RSSI for ongoing alert intensity
                return true;
            }
            // Cooldown expired, update timestamp
            trackedDevices[i].lastSeen = now;
            return false;
        }
    }
    return false;
}

void trackDevice(const uint8_t* mac, int rssi, uint8_t tier, bool hasCamera) {
    uint32_t now = millis();

    // Update existing entry
    for (int i = 0; i < trackedDeviceCount; i++) {
        if (memcmp(trackedDevices[i].mac, mac, 6) == 0) {
            trackedDevices[i].lastSeen = now;
            trackedDevices[i].rssi = rssi;
            return;
        }
    }

    // Add new entry (evict oldest if full)
    if (trackedDeviceCount >= MAX_TRACKED_DEVICES) {
        // Find oldest entry
        int oldest = 0;
        for (int i = 1; i < trackedDeviceCount; i++) {
            if (trackedDevices[i].lastSeen < trackedDevices[oldest].lastSeen) {
                oldest = i;
            }
        }
        trackedDevices[oldest] = { {0}, now, rssi, tier, hasCamera };
        memcpy(trackedDevices[oldest].mac, mac, 6);
    } else {
        memset(&trackedDevices[trackedDeviceCount], 0, sizeof(TrackedDevice));
        memcpy(trackedDevices[trackedDeviceCount].mac, mac, 6);
        trackedDevices[trackedDeviceCount].lastSeen = now;
        trackedDevices[trackedDeviceCount].rssi = rssi;
        trackedDevices[trackedDeviceCount].tier = tier;
        trackedDevices[trackedDeviceCount].hasCamera = hasCamera;
        trackedDeviceCount++;
    }
}

// ============================================================
// Detection Engine
// ============================================================

// Convert raw bytes to uppercase hex string
String bytesToHex(const uint8_t* data, size_t len) {
    String hex = "";
    for (size_t i = 0; i < len; i++) {
        if (data[i] < 0x10) hex += "0";
        hex += String(data[i], HEX);
    }
    hex.toUpperCase();
    return hex;
}

// Case-insensitive substring search
bool containsIgnoreCase(const String& haystack, const char* needle) {
    String lower = haystack;
    lower.toLowerCase();
    String needleLower = String(needle);
    needleLower.toLowerCase();
    return lower.indexOf(needleLower) >= 0;
}

struct DetectionResult {
    bool        detected;
    const char* company;
    const char* product;
    const char* reason;
    bool        hasCamera;
    uint8_t     tier;
    char        reasonBuf[128];
};

// Check company ID against database
bool checkCompanyID(uint16_t companyId, DetectionResult& result) {
    for (int i = 0; i < GLASSES_COMPANY_ID_COUNT; i++) {
        const GlassesCompanyID& entry = GLASSES_COMPANY_IDS[i];
        if (entry.id == companyId) {
            // Check tier enablement
            if (entry.tier == TIER_HIGH && !ENABLE_TIER_HIGH) continue;
            if (entry.tier == TIER_MEDIUM && !ENABLE_TIER_MEDIUM) continue;
            if (entry.tier == TIER_LOW && !ENABLE_TIER_LOW) continue;

            result.detected = true;
            result.company = entry.company;
            result.product = entry.product;
            result.hasCamera = entry.hasCamera;
            result.tier = entry.tier;
            snprintf(result.reasonBuf, sizeof(result.reasonBuf),
                     "Company ID 0x%04X (%s)", companyId, entry.company);
            result.reason = result.reasonBuf;
            return true;
        }
    }
    return false;
}

// Check service UUIDs
bool checkServiceUUIDs(BLEAdvertisedDevice& device, DetectionResult& result) {
    for (int i = 0; GLASSES_SERVICE_UUIDS[i].uuid16 != 0; i++) {
        BLEUUID targetUUID(GLASSES_SERVICE_UUIDS[i].uuid16);
        if (device.isAdvertisingService(targetUUID)) {
            result.detected = true;
            result.company = GLASSES_SERVICE_UUIDS[i].owner;
            result.product = GLASSES_SERVICE_UUIDS[i].description;
            result.hasCamera = true;
            result.tier = TIER_HIGH;
            snprintf(result.reasonBuf, sizeof(result.reasonBuf),
                     "Service UUID 0x%04X (%s)",
                     GLASSES_SERVICE_UUIDS[i].uuid16,
                     GLASSES_SERVICE_UUIDS[i].owner);
            result.reason = result.reasonBuf;
            return true;
        }
    }
    return false;
}

// Check device name patterns
bool checkDeviceName(const String& name, DetectionResult& result) {
    if (name.length() == 0) return false;

    for (int i = 0; GLASSES_NAME_PATTERNS[i].pattern != NULL; i++) {
        if (containsIgnoreCase(name, GLASSES_NAME_PATTERNS[i].pattern)) {
            result.detected = true;
            result.company = GLASSES_NAME_PATTERNS[i].product;
            result.product = GLASSES_NAME_PATTERNS[i].product;
            result.hasCamera = GLASSES_NAME_PATTERNS[i].hasCamera;
            result.tier = TIER_HIGH;  // Name match is high confidence
            snprintf(result.reasonBuf, sizeof(result.reasonBuf),
                     "Device name '%s' matches '%s'",
                     name.c_str(), GLASSES_NAME_PATTERNS[i].pattern);
            result.reason = result.reasonBuf;
            return true;
        }
    }
    return false;
}

// Check MAC OUI prefix (supplementary — BLE MACs can be random)
bool checkOUIPrefix(const uint8_t* mac, DetectionResult& result) {
    for (int i = 0; GLASSES_OUI_PREFIXES[i].vendor != NULL; i++) {
        if (memcmp(mac, GLASSES_OUI_PREFIXES[i].oui, 3) == 0) {
            result.detected = true;
            result.company = GLASSES_OUI_PREFIXES[i].vendor;
            result.product = "Smart Glasses (OUI match)";
            result.hasCamera = true;
            result.tier = TIER_MEDIUM;  // OUI is supplementary
            snprintf(result.reasonBuf, sizeof(result.reasonBuf),
                     "OUI prefix %02X:%02X:%02X (%s)",
                     mac[0], mac[1], mac[2],
                     GLASSES_OUI_PREFIXES[i].vendor);
            result.reason = result.reasonBuf;
            return true;
        }
    }
    return false;
}

// ============================================================
// Serial JSON Output
// ============================================================

void sendDetectionJSON(BLEAdvertisedDevice& device, const DetectionResult& result) {
    JsonDocument doc;
    doc["type"] = "detection";
    doc["mac"] = device.getAddress().toString().c_str();
    doc["company"] = result.company;
    doc["product"] = result.product;
    doc["reason"] = result.reason;
    doc["rssi"] = device.getRSSI();
    doc["hasCamera"] = result.hasCamera;
    doc["tier"] = result.tier;

    if (device.haveName()) {
        doc["deviceName"] = device.getName().c_str();
    }

    if (device.haveManufacturerData()) {
        std::string mfgData = device.getManufacturerData();
        if (mfgData.length() >= 2) {
            uint16_t cid = (uint8_t)mfgData[0] | ((uint8_t)mfgData[1] << 8);
            char cidHex[7];
            snprintf(cidHex, sizeof(cidHex), "0x%04X", cid);
            doc["companyId"] = cidHex;
        }
    }

    doc["ts"] = millis();

    serializeJson(doc, Serial);
    Serial.println();
}

void sendStatusJSON() {
    JsonDocument doc;
    doc["type"] = "status";
    doc["board"] = BOARD_TYPE;
    doc["uptime"] = millis() / 1000;
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["totalScans"] = totalScans;
    doc["totalDetections"] = totalDetections;
    doc["trackedDevices"] = trackedDeviceCount;
    doc["alertActive"] = alertActive;
    doc["tierHigh"] = ENABLE_TIER_HIGH;
    doc["tierMedium"] = ENABLE_TIER_MEDIUM;
    doc["tierLow"] = ENABLE_TIER_LOW;
    doc["rssiThreshold"] = RSSI_THRESHOLD_DEFAULT;

    serializeJson(doc, Serial);
    Serial.println();
}

void sendHeartbeatJSON() {
    JsonDocument doc;
    doc["type"] = "heartbeat";
    doc["uptime"] = millis() / 1000;
    doc["freeHeap"] = ESP.getFreeHeap();

    serializeJson(doc, Serial);
    Serial.println();
}

// ============================================================
// BLE Scan Callback
// ============================================================

class GlassholeScanCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) override {
        int rssi = advertisedDevice.getRSSI();

        // RSSI gate — ignore weak signals
        if (rssi < RSSI_THRESHOLD_DEFAULT) return;

        DetectionResult result = {};
        bool detected = false;

        // 1. Check manufacturer-specific company ID (primary method)
        if (!detected && advertisedDevice.haveManufacturerData()) {
            std::string mfgData = advertisedDevice.getManufacturerData();
            if (mfgData.length() >= 2) {
                uint16_t companyId = (uint8_t)mfgData[0] | ((uint8_t)mfgData[1] << 8);
                detected = checkCompanyID(companyId, result);
            }
        }

        // 2. Check service UUIDs
        if (!detected) {
            detected = checkServiceUUIDs(advertisedDevice, result);
        }

        // 3. Check device name patterns
        if (!detected && advertisedDevice.haveName()) {
            String name = advertisedDevice.getName().c_str();
            detected = checkDeviceName(name, result);
        }

        // 4. Check MAC OUI prefix (supplementary)
        if (!detected) {
            const uint8_t* nativeAddr = *advertisedDevice.getAddress().getNative();
            detected = checkOUIPrefix(nativeAddr, result);
        }

        if (!detected) return;

        // --- Detection confirmed ---

        // Get MAC for deduplication
        const uint8_t* mac = *advertisedDevice.getAddress().getNative();

        // Check cooldown
        if (isDeviceCoolingDown(mac)) return;

        // Track this device
        trackDevice(mac, rssi, result.tier, result.hasCamera);

        // Update counters
        totalDetections++;

        // Trigger LED alert
        triggerAlert(rssi, result.tier, result.hasCamera);

        // Send JSON to serial
        sendDetectionJSON(advertisedDevice, result);
    }
};

// ============================================================
// Setup
// ============================================================

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(500);

    // Configure LED
#if !HAS_RGB_LED
    pinMode(LED_PIN, OUTPUT);
#endif
    ledOff();

    // Boot banner
    Serial.println();
    Serial.println("========================================");
    Serial.println("  ESP-GlassHole — AR Glasses Detector");
    Serial.println("========================================");
    Serial.printf("  Board:  %s\n", BOARD_TYPE);
    Serial.printf("  LED:    GPIO %d (%s)\n", LED_PIN, HAS_RGB_LED ? "RGB" : "standard");
    Serial.printf("  RSSI:   %d dBm threshold\n", RSSI_THRESHOLD_DEFAULT);
    Serial.printf("  Tiers:  HIGH=%s MEDIUM=%s LOW=%s\n",
                  ENABLE_TIER_HIGH ? "ON" : "OFF",
                  ENABLE_TIER_MEDIUM ? "ON" : "OFF",
                  ENABLE_TIER_LOW ? "ON" : "OFF");
    Serial.printf("  DB:     %d company IDs, %d OUI prefixes\n",
                  GLASSES_COMPANY_ID_COUNT,
                  (int)(sizeof(GLASSES_OUI_PREFIXES) / sizeof(GLASSES_OUI_PREFIXES[0])) - 1);
    Serial.println("========================================");
    Serial.println();

    // Initialize BLE
    BLEDevice::init("ESP-GlassHole");
    pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new GlassholeScanCallbacks(), true);
    pBLEScan->setActiveScan(true);
    pBLEScan->setInterval(160);   // 100ms in 0.625ms units
    pBLEScan->setWindow(128);     // 80ms in 0.625ms units

    // Boot flash — 3 quick blinks to show we're alive
    for (int i = 0; i < 3; i++) {
        ledOn();
        delay(100);
        ledOff();
        delay(100);
    }
    ledIdle();

    Serial.println("{\"type\":\"boot\",\"board\":\"" BOARD_TYPE "\",\"version\":\"2.0.0\"}");

    lastStatusTime = millis();
    lastHeartbeatTime = millis();
}

// ============================================================
// Main Loop
// ============================================================

void loop() {
    // Run BLE scan (non-blocking via callback)
    BLEScanResults results = pBLEScan->start(BLE_SCAN_TIME, false);
    totalScans++;

    // Clear scan results to free memory
    pBLEScan->clearResults();

    // Update LED state
    updateLED();

    // Periodic status
    uint32_t now = millis();
    if (now - lastStatusTime >= STATUS_INTERVAL_MS) {
        sendStatusJSON();
        lastStatusTime = now;
    }

    // Heartbeat
    if (now - lastHeartbeatTime >= HEARTBEAT_INTERVAL_MS) {
        sendHeartbeatJSON();
        lastHeartbeatTime = now;
    }

    // Brief pause between scans
    delay(100);

    // Update LED between scans
    updateLED();
}
