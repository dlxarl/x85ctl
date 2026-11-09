#pragma once
#include "protocol/CommandBuilder.h"
#include <QColor>
#include <QMap>
#include <algorithm>
#include <cmath>

namespace kcc {
// Maps the colour the user picks (sRGB, as seen on screen) to the PWM values sent to the LEDs.
// Keyboard LEDs are linear and green/blue dies are much brighter than red, so raw sRGB looks
// washed-out and green-tinted. The vendor app sends raw values (no correction in DeviceDriver.exe).
//   out = 255 * gain_c * (in / 255) ^ gamma
// Defaults: gamma 1.0 (2.2 crushes dim channels, so gradients collapse to one hue) and FastLED's "TypicalSMD5050" white balance
// 255/176/240 (gains 1.00 / 0.69 / 0.94). Users can calibrate white in the Lighting page.
struct ColorCorrection {
    bool enabled = true;
    double gamma = 1.0;
    double gainR = 1.0, gainG = 176.0 / 255.0, gainB = 240.0 / 255.0;

    static uint8_t channel(uint8_t v, double gamma, double gain) {
        if (v == 0) return 0;
        const double o = 255.0 * gain * std::pow(v / 255.0, gamma);
        return uint8_t(std::clamp(std::lround(o), 1L, 255L));   // a lit channel never rounds to off
    }
    Rgb apply(Rgb c) const {
        if (!enabled) return c;
        return {channel(c.r, gamma, gainR), channel(c.g, gamma, gainG), channel(c.b, gamma, gainB)};
    }
    // Inverse, for colours read back from the board (04 F5) so the editor shows what was picked.
    static uint8_t unchannel(uint8_t v, double gamma, double gain) {
        if (v == 0) return 0;
        const double x = std::min(1.0, v / (255.0 * gain));
        return uint8_t(std::clamp(std::lround(255.0 * std::pow(x, 1.0 / gamma)), 1L, 255L));
    }
    Rgb invert(Rgb c) const {
        if (!enabled) return c;
        return {unchannel(c.r, gamma, gainR), unchannel(c.g, gamma, gainG), unchannel(c.b, gamma, gainB)};
    }
    QMap<int, Rgb> apply(const QMap<int, Rgb>& m) const {
        QMap<int, Rgb> out;
        for (auto it = m.begin(); it != m.end(); ++it) out.insert(it.key(), apply(it.value()));
        return out;
    }
};
}
