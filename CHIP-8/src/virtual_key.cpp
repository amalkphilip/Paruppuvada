#include "virtual_key.h"

namespace retro_gui {

const char* virtualKeyName(VirtualKey key) {
    switch (key) {
        case VirtualKey::KEY_1: return "1";
        case VirtualKey::KEY_2: return "2";
        case VirtualKey::KEY_3: return "3";
        case VirtualKey::KEY_C: return "C";
        case VirtualKey::KEY_4: return "4";
        case VirtualKey::KEY_5: return "5";
        case VirtualKey::KEY_6: return "6";
        case VirtualKey::KEY_D: return "D";
        case VirtualKey::KEY_7: return "7";
        case VirtualKey::KEY_8: return "8";
        case VirtualKey::KEY_9: return "9";
        case VirtualKey::KEY_E: return "E";
        case VirtualKey::KEY_A: return "A";
        case VirtualKey::KEY_0: return "0";
        case VirtualKey::KEY_B: return "B";
        case VirtualKey::KEY_F: return "F";
        case VirtualKey::ARROW_UP: return "UP";
        case VirtualKey::ARROW_DOWN: return "DOWN";
        case VirtualKey::ARROW_LEFT: return "LEFT";
        case VirtualKey::ARROW_RIGHT: return "RIGHT";
        case VirtualKey::SPACE: return "SPACE";
        case VirtualKey::SAVE: return "SAVE";
        case VirtualKey::LOAD: return "LOAD";
        case VirtualKey::COUNT: break;
    }
    return "";
}

int virtualKeyToChip8Hex(VirtualKey key) {
    switch (key) {
        case VirtualKey::KEY_0: return 0x0;
        case VirtualKey::KEY_1: return 0x1;
        case VirtualKey::KEY_2: return 0x2;
        case VirtualKey::KEY_3: return 0x3;
        case VirtualKey::KEY_4: return 0x4;
        case VirtualKey::KEY_5: return 0x5;
        case VirtualKey::KEY_6: return 0x6;
        case VirtualKey::KEY_7: return 0x7;
        case VirtualKey::KEY_8: return 0x8;
        case VirtualKey::KEY_9: return 0x9;
        case VirtualKey::KEY_A: return 0xA;
        case VirtualKey::KEY_B: return 0xB;
        case VirtualKey::KEY_C: return 0xC;
        case VirtualKey::KEY_D: return 0xD;
        case VirtualKey::KEY_E: return 0xE;
        case VirtualKey::KEY_F: return 0xF;
        default: return -1;
    }
}

} // namespace retro_gui
