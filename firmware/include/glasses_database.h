/*
 * ESP-GlassHole — AR/Smart Glasses Detection Database
 *
 * Company IDs from Bluetooth SIG Assigned Numbers registry.
 * OUI prefixes from IEEE MA-L registry + field captures.
 * Device name patterns from BLE advertisement analysis.
 *
 * Sources:
 *   - Bluetooth SIG: bluetooth.com/specifications/assigned-numbers/
 *   - Nordic Semiconductor DB: github.com/NordicSemiconductor/bluetooth-numbers-database
 *   - yj_nearbyglasses: github.com/yjeanrenaud/yj_nearbyglasses
 *   - glass-detect: github.com/sh4d0wm45k/glass-detect
 *   - banrays: github.com/NullPxl/banrays
 *   - ouispy-detector: github.com/colonelpanichacks/ouispy-detector
 *
 * License: AGPL-3.0 (inherited from upstream)
 */

#ifndef GLASSES_DATABASE_H
#define GLASSES_DATABASE_H

#include <stdint.h>
#include <stddef.h>

// ============================================================
// Detection Tiers
// ============================================================
// HIGH:   Dedicated glasses/eyewear companies, low false-positive risk
// MEDIUM: Companies with glasses products, moderate FP risk
// LOW:    Large ecosystems where glasses are one of many BLE products

#define TIER_HIGH   0
#define TIER_MEDIUM 1
#define TIER_LOW    2

// ============================================================
// BLE Company ID Database
// ============================================================
// Company IDs appear in BLE Manufacturer Specific Data (AD Type 0xFF).
// First 2 bytes (little-endian) of manufacturer data = company ID.
// These are mandatory and immutable per Bluetooth spec.

struct GlassesCompanyID {
    uint16_t    id;
    const char* company;
    const char* product;
    bool        hasCamera;
    uint8_t     tier;
};

static const GlassesCompanyID GLASSES_COMPANY_IDS[] = {
    // --- TIER HIGH: Dedicated smart glasses companies ---
    { 0x01AB, "Meta Platforms",              "Ray-Ban Meta",           true,  TIER_HIGH },
    { 0x058E, "Meta Platforms Technologies", "Ray-Ban Meta / Quest",   true,  TIER_HIGH },
    { 0x0D53, "Luxottica Group",             "Ray-Ban / Oakley Meta",  true,  TIER_HIGH },
    { 0x03C2, "Snapchat Inc",               "Snap Spectacles",        true,  TIER_HIGH },
    { 0x060C, "Vuzix Corporation",           "Vuzix Blade / Shield",   true,  TIER_HIGH },

    // --- TIER MEDIUM: Known glasses, moderate FP risk ---
    { 0x00E0, "Google",                      "Google Glass",           true,  TIER_MEDIUM },
    { 0x018E, "Google LLC",                  "Google Glass EE2",       true,  TIER_MEDIUM },
    { 0x0BC6, "TCL Communication",           "RayNeo X2 / Air",       true,  TIER_MEDIUM },
    { 0x03AB, "Meizu Technology",            "MYVU AR Glasses",        true,  TIER_MEDIUM },
    { 0x0562, "Thalmic Labs (North)",        "North Focals",           false, TIER_MEDIUM },
    { 0x041F, "Kopin Corporation",           "Solos AirGo",            false, TIER_MEDIUM },
    { 0x012D, "Sony Corporation",            "SmartEyeglass",          true,  TIER_MEDIUM },
    { 0x0040, "Seiko Epson",                 "Moverio BT Series",     true,  TIER_MEDIUM },

    // --- TIER LOW: Large ecosystems, high FP risk ---
    { 0x004C, "Apple",                       "Vision Pro",             true,  TIER_LOW },
    { 0x038F, "Xiaomi",                      "AI Glasses",             true,  TIER_LOW },
    { 0x079A, "Oppo",                        "Air Glass",              true,  TIER_LOW },
    { 0x027D, "Huawei Technologies",         "Eyewear",                false, TIER_LOW },
    { 0x0075, "Samsung Electronics",         "AR Glasses (future)",    false, TIER_LOW },
    { 0x0171, "Amazon",                      "Echo Frames",            false, TIER_LOW },
    { 0x009E, "Bose Corporation",            "Bose Frames",            false, TIER_LOW },
    { 0x02C5, "Lenovo",                      "ThinkReality A3",        true,  TIER_LOW },
    { 0x02ED, "HTC Corporation",             "Vive XR Elite",          true,  TIER_LOW },
    { 0x0B24, "ByteDance",                   "Pico XR",                true,  TIER_LOW },
    { 0x068E, "Razer Inc",                   "Razer Anzu",             false, TIER_LOW },
    { 0x0976, "Fauna Audio",                 "Fauna Glasses",          false, TIER_LOW },

    // --- End sentinel ---
    { 0x0000, NULL, NULL, false, 0 }
};

static const int GLASSES_COMPANY_ID_COUNT =
    (sizeof(GLASSES_COMPANY_IDS) / sizeof(GLASSES_COMPANY_IDS[0])) - 1;

// ============================================================
// BLE Service UUIDs associated with smart glasses
// ============================================================

struct GlassesServiceUUID {
    uint16_t    uuid16;
    const char* owner;
    const char* description;
};

static const GlassesServiceUUID GLASSES_SERVICE_UUIDS[] = {
    { 0xFD5F, "Meta Platforms",  "Meta BLE Service (Ray-Ban)" },
    { 0xFEAA, "Google",          "Eddystone (Glass beacon)" },
    { 0x0000, NULL, NULL }
};

// ============================================================
// BLE OUI Prefixes (first 3 bytes of MAC address)
// ============================================================
// NOTE: BLE MAC addresses can be randomized, making OUI detection
// unreliable for BLE. These work better for WiFi detection.
// Included as a secondary heuristic — match adds confidence
// but should not be the sole detection method.

struct GlassesOUI {
    uint8_t     oui[3];
    const char* vendor;
};

static const GlassesOUI GLASSES_OUI_PREFIXES[] = {
    // Meta Platforms Technologies (from glass-detect + ouispy-detector)
    { { 0x7C, 0x2A, 0x9E }, "Meta Platforms Technologies" },
    { { 0xCC, 0x66, 0x0A }, "Meta Platforms Technologies" },
    { { 0xF4, 0x03, 0x43 }, "Meta Platforms Technologies" },
    { { 0x5C, 0xE9, 0x1E }, "Meta Platforms Technologies" },
    // Luxottica (manufacturer of Ray-Ban frames)
    { { 0x98, 0x59, 0x49 }, "Luxottica Group" },
    // End sentinel
    { { 0x00, 0x00, 0x00 }, NULL }
};

// ============================================================
// BLE Device Name Patterns (case-insensitive substring match)
// ============================================================

struct GlassesNamePattern {
    const char* pattern;
    const char* product;
    bool        hasCamera;
};

static const GlassesNamePattern GLASSES_NAME_PATTERNS[] = {
    { "rayban",      "Meta Ray-Ban",       true },
    { "ray-ban",     "Meta Ray-Ban",       true },
    { "ray ban",     "Meta Ray-Ban",       true },
    { "meta_rb",     "Meta Ray-Ban",       true },   // from manufacturer data decode
    { "spectacles",  "Snap Spectacles",    true },
    { "vuzix",       "Vuzix Glasses",      true },
    { "moverio",     "Epson Moverio",      true },
    { "focals",      "North Focals",       false },
    { "echo frames", "Amazon Echo Frames", false },
    { "rayneo",      "TCL RayNeo",         true },
    { "solos",       "Solos AirGo",        false },
    { "even g",      "Even Realities",     false },
    { "inmo",        "INMO Air",           false },
    { "rokid",       "Rokid Glasses",      false },
    { "xreal",       "Xreal Air",          false },
    { "nreal",       "Xreal (Nreal)",      false },
    { NULL, NULL, false }
};

// ============================================================
// Manufacturer Data Fingerprints
// ============================================================
// Specific byte sequences in manufacturer data that identify
// glasses vs other products from the same company.

struct GlassesMfgDataPattern {
    uint16_t    companyId;
    const char* hexPattern;    // hex string to search for in mfg data
    const char* description;
};

static const GlassesMfgDataPattern GLASSES_MFG_DATA_PATTERNS[] = {
    // "META_RB_GLASS" in hex (from banrays project captures)
    { 0x058E, "4D455441_52425F474C415353", "Meta Ray-Ban (META_RB_GLASS)" },
    { 0x0000, NULL, NULL }
};

#endif // GLASSES_DATABASE_H
