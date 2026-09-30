#ifndef RETRO_GUI_RETRO_THEME_H
#define RETRO_GUI_RETRO_THEME_H

#include <cstdint>

namespace retro_gui {

struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

Color rgb(uint8_t r, uint8_t g, uint8_t b);
Color rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

// Physical case theme. This is independent from the CRT display palette.
enum class ComputerTheme : uint8_t {
    WHITE = 0,
    GREEN,
    AMBER,
    CHARCOAL,
    COUNT
};

// CRT/emulator display palette. It never recolors the physical computer.
enum class PaletteID : uint8_t {
    CLASSIC_GREEN = 0,
    AMBER_CRT,
    NEON_HIGH_CONTRAST,
    COUNT
};

struct ComputerThemeDef {
    Color background;
    Color backgroundDots;
    Color bodyDark;
    Color bodyMid;
    Color bodyLight;
    Color bodyEdge;
    Color bezel;
    Color bezelDark;
    Color glass;
    Color keyFace;
    Color keyTop;
    Color keySide;
    Color keyLabel;
    Color accent;
    Color textMain;
};

struct DisplayPaletteDef {
    Color background;
    Color foreground;
    Color dim;
    Color glass;
    Color glow;
};

const char* computerThemeName(ComputerTheme theme);
const char* paletteName(PaletteID palette);

ComputerTheme nextComputerTheme(ComputerTheme theme);
PaletteID nextPalette(PaletteID palette);
PaletteID previousPalette(PaletteID palette);

ComputerThemeDef builtinComputerTheme(ComputerTheme theme);
DisplayPaletteDef builtinDisplayPalette(PaletteID palette);

} // namespace retro_gui

#endif // RETRO_GUI_RETRO_THEME_H
