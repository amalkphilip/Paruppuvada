#include "retro_theme.h"

namespace retro_gui {

Color rgb(uint8_t r, uint8_t g, uint8_t b) {
    return Color{r, g, b, 255};
}

Color rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return Color{r, g, b, a};
}

const char* computerThemeName(ComputerTheme theme) {
    switch (theme) {
        case ComputerTheme::WHITE: return "WHITE";
        case ComputerTheme::GREEN: return "GREEN";
        case ComputerTheme::AMBER: return "AMBER";
        case ComputerTheme::CHARCOAL: return "CHARCOAL";
        case ComputerTheme::COUNT: break;
    }
    return "UNKNOWN";
}

const char* paletteName(PaletteID palette) {
    switch (palette) {
        case PaletteID::CLASSIC_GREEN: return "GREEN";
        case PaletteID::AMBER_CRT: return "AMBER";
        case PaletteID::NEON_HIGH_CONTRAST: return "NEON";
        case PaletteID::COUNT: break;
    }
    return "UNKNOWN";
}

ComputerTheme nextComputerTheme(ComputerTheme theme) {
    const int count = static_cast<int>(ComputerTheme::COUNT);
    return static_cast<ComputerTheme>((static_cast<int>(theme) + 1) % count);
}

PaletteID nextPalette(PaletteID palette) {
    const int count = static_cast<int>(PaletteID::COUNT);
    return static_cast<PaletteID>((static_cast<int>(palette) + 1) % count);
}

PaletteID previousPalette(PaletteID palette) {
    const int count = static_cast<int>(PaletteID::COUNT);
    return static_cast<PaletteID>((static_cast<int>(palette) + count - 1) % count);
}

ComputerThemeDef builtinComputerTheme(ComputerTheme theme) {
    ComputerThemeDef t{};
    switch (theme) {
        case ComputerTheme::GREEN:
            t.background     = rgb(0x07, 0x11, 0x08);
            t.backgroundDots = rgb(0x0B, 0x1F, 0x10);
            t.bodyDark       = rgb(0x0B, 0x1F, 0x10);
            t.bodyMid        = rgb(0x12, 0x3D, 0x1C);
            t.bodyLight      = rgb(0x1E, 0x57, 0x2B);
            t.bodyEdge       = rgb(0x39, 0xA9, 0x4A);
            t.bezel          = rgb(0x0E, 0x2E, 0x16);
            t.bezelDark      = rgb(0x07, 0x11, 0x08);
            t.glass          = rgb(0x08, 0x16, 0x0A);
            t.keyFace        = rgb(0x1A, 0x4A, 0x24);
            t.keyTop         = rgb(0x39, 0xA9, 0x4A);
            t.keySide        = rgb(0x0B, 0x1F, 0x10);
            t.keyLabel       = rgb(0x8F, 0xEA, 0x9B);
            t.accent         = rgb(0x8F, 0xEA, 0x9B);
            t.textMain       = rgb(0x8F, 0xEA, 0x9B);
            break;
        case ComputerTheme::AMBER:
            t.background     = rgb(0x1B, 0x12, 0x08);
            t.backgroundDots = rgb(0x28, 0x1A, 0x0C);
            t.bodyDark       = rgb(0x3B, 0x24, 0x12);
            t.bodyMid        = rgb(0x5A, 0x36, 0x1A);
            t.bodyLight      = rgb(0x8C, 0x54, 0x28);
            t.bodyEdge       = rgb(0xA4, 0x6E, 0x39);
            t.bezel          = rgb(0x28, 0x18, 0x0B);
            t.bezelDark      = rgb(0x15, 0x0D, 0x05);
            t.glass          = rgb(0x0C, 0x08, 0x03);
            t.keyFace        = rgb(0x5A, 0x36, 0x1A);
            t.keyTop         = rgb(0xA4, 0x6E, 0x39);
            t.keySide        = rgb(0x3B, 0x24, 0x12);
            t.keyLabel       = rgb(0xFF, 0xB2, 0x45);
            t.accent         = rgb(0xFF, 0xB2, 0x45);
            t.textMain       = rgb(0xFF, 0xB2, 0x45);
            break;
        case ComputerTheme::CHARCOAL:
            t.background     = rgb(0x11, 0x11, 0x11);
            t.backgroundDots = rgb(0x1A, 0x1A, 0x1A);
            t.bodyDark       = rgb(0x22, 0x22, 0x22);
            t.bodyMid        = rgb(0x33, 0x33, 0x33);
            t.bodyLight      = rgb(0x4A, 0x4A, 0x4A);
            t.bodyEdge       = rgb(0x66, 0x66, 0x66);
            t.bezel          = rgb(0x11, 0x11, 0x11);
            t.bezelDark      = rgb(0x0A, 0x0A, 0x0A);
            t.glass          = rgb(0x05, 0x05, 0x05);
            t.keyFace        = rgb(0x2A, 0x2A, 0x2A);
            t.keyTop         = rgb(0x55, 0x55, 0x55);
            t.keySide        = rgb(0x1A, 0x1A, 0x1A);
            t.keyLabel       = rgb(0xAA, 0xAA, 0xAA);
            t.accent         = rgb(0x88, 0x88, 0x88);
            t.textMain       = rgb(0xAA, 0xAA, 0xAA);
            break;
        case ComputerTheme::WHITE:
        default:
            // Required default white retro theme.
            t.background     = rgb(0x22, 0x22, 0x20);
            t.backgroundDots = rgb(0x2C, 0x2B, 0x28);
            t.bodyDark       = rgb(0xA7, 0xA3, 0x9A);
            t.bodyMid        = rgb(0xE7, 0xE3, 0xD8);
            t.bodyLight      = rgb(0xF5, 0xF2, 0xE8);
            t.bodyEdge       = rgb(0x66, 0x63, 0x5E);
            t.bezel          = rgb(0xCF, 0xCA, 0xBE);
            t.bezelDark      = rgb(0x5B, 0x59, 0x54);
            t.glass          = rgb(0x13, 0x18, 0x14);
            t.keyFace        = rgb(0xE9, 0xE7, 0xDF);
            t.keyTop         = rgb(0xFF, 0xFF, 0xFF);
            t.keySide        = rgb(0x9A, 0x96, 0x8E);
            t.keyLabel       = rgb(0x30, 0x30, 0x2D);
            t.accent         = rgb(0x43, 0x76, 0x55);
            t.textMain       = rgb(0x30, 0x30, 0x2D);
            break;
    }
    return t;
}

DisplayPaletteDef builtinDisplayPalette(PaletteID palette) {
    DisplayPaletteDef p{};
    switch (palette) {
        case PaletteID::AMBER_CRT:
            p.background = rgb(0x18, 0x0D, 0x03);
            p.foreground = rgb(0xFF, 0xB2, 0x45);
            p.dim        = rgb(0x8B, 0x55, 0x1B);
            p.glass      = rgb(0x12, 0x0A, 0x03);
            p.glow       = rgb(0xFF, 0xB2, 0x45);
            break;
        case PaletteID::NEON_HIGH_CONTRAST:
            p.background = rgb(0x02, 0x04, 0x03);
            p.foreground = rgb(0xD7, 0xFF, 0x3B);
            p.dim        = rgb(0x37, 0x89, 0x4A);
            p.glass      = rgb(0x03, 0x07, 0x05);
            p.glow       = rgb(0xD7, 0xFF, 0x3B);
            break;
        case PaletteID::CLASSIC_GREEN:
        default:
            p.background = rgb(0x0B, 0x1F, 0x10);
            p.foreground = rgb(0x8F, 0xEA, 0x9B);
            p.dim        = rgb(0x39, 0xA9, 0x4A);
            p.glass      = rgb(0x08, 0x16, 0x0A);
            p.glow       = rgb(0x39, 0xA9, 0x4A);
            break;
    }
    return p;
}

} // namespace retro_gui
