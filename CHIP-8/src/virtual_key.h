#ifndef RETRO_GUI_VIRTUAL_KEY_H
#define RETRO_GUI_VIRTUAL_KEY_H

#include <cstdint>

namespace retro_gui {

// Generic GUI-level key identity. The GUI does not assign game semantics;
// the host maps these values to CHIP-8 keys or game actions.
enum class VirtualKey : uint8_t {
    KEY_1 = 0,
    KEY_2,
    KEY_3,
    KEY_C,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_D,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_E,
    KEY_A,
    KEY_0,
    KEY_B,
    KEY_F,

    ARROW_UP,
    ARROW_DOWN,
    ARROW_LEFT,
    ARROW_RIGHT,

    SPACE,
    SAVE,
    LOAD,

    COUNT
};

constexpr int virtualKeyCount() {
    return static_cast<int>(VirtualKey::COUNT);
}

const char* virtualKeyName(VirtualKey key);

// Returns the CHIP-8 hexadecimal value for KEY_0..KEY_F, or -1 for controls
// that are intentionally not part of the original hexadecimal keypad.
int virtualKeyToChip8Hex(VirtualKey key);

} // namespace retro_gui

#endif // RETRO_GUI_VIRTUAL_KEY_H
