#pragma once
#include <array>
#include <cstdint>

// Protocol constants for SONIX 0C45:8006 (ATTACK SHARK X85).
// Every value here is taken from the decompiled vendor app; see docs/PROTOCOL.md.
namespace kcc::x85 {

inline constexpr uint16_t kVid = 0x0C45;
inline constexpr uint16_t kPid = 0x8006;
inline constexpr uint16_t kUsagePage = 0xFF13;
inline constexpr uint16_t kUsage = 0x0001;

inline constexpr int kPacketSize = 64;       // payload, sent with report ID 0
inline constexpr int kCmdDelayMs = 35;        // <cmd_delaytime value="35"/>
inline constexpr int kSlotCount = 144;        // 576-byte tables / 4
inline constexpr int kTableBytes = kSlotCount * 4;

inline constexpr uint8_t kPrefix = 0x04;
enum Cmd : uint8_t {
    End = 0x02,
    LightMode = 0x13,
    Macro = 0x15,
    Settings = 0x17,
    Begin = 0x18,
    BeginMacro = 0x19,
    KeymapBase = 0x11,
    KeymapFn = 0x27,
    PerKeyColor = 0x23,
    ReadPerKeyColor = 0xF5,
    Apply = 0xF0,
};

enum class Direction : uint8_t { Left = 0, Right = 1, Up = 2, Down = 3 };

// config_func bits from the vendor mode table (FUN_0042ae40).
enum ModeCaps : uint8_t {
    CapBrightness = 0x01,
    CapSpeed = 0x02,
    CapDirectionLR = 0x04,
    CapDirectionUD = 0x08,
    CapRgb = 0x10,
    CapColorful = 0x20,
};

struct ModeDef { uint8_t id; const char* name; uint8_t caps; };
inline constexpr std::array<ModeDef, 20> kModes{{
    {1, "Static", 0x31},   {2, "Single On", 0x33}, {3, "Single Off", 0x33}, {4, "Glittering", 0x33},
    {5, "Falling", 0x33},  {6, "Colourful", 0x03}, {7, "Breath", 0x33},     {8, "Spectrum", 0x03},
    {9, "Outward", 0x33},  {10, "Scrolling", 0x3B}, {11, "Rolling", 0x37},  {12, "Rotating", 0x37},
    {13, "Explode", 0x33}, {14, "Launch", 0x33},   {15, "Ripples", 0x33},   {16, "Flowing", 0x37},
    {17, "Pulsating", 0x33}, {18, "Tilt", 0x37},   {19, "Shuttle", 0x33},   {0, "Off", 0x00},
}};
inline constexpr uint8_t kModeUser = 0x80;    // per-key colours
inline constexpr int kBrightnessMax = 5;
inline constexpr int kSpeedMax = 5;
inline constexpr uint8_t kDefaultMode = 11, kDefaultBrightness = 5, kDefaultSpeed = 3;

const ModeDef* findMode(uint8_t id);
}
