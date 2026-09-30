# Paruppuvada PC-8 — CHIP-8 Emulator

A C++/SDL2 CHIP-8 emulator presented as a retro-computer experience. The project contains the CHIP-8 virtual machine, SDL2 graphics/audio, ROMs, save-state support, and a custom `RetroComputerUI` that presents the emulator through a physical-looking 3D pixel-art computer interface.

## Overview

CHIP-8 is a small virtual machine originally designed for simple games on early microcomputers. This project implements the CHIP-8 architecture and provides a desktop emulator for running `.ch8` ROMs.

The project also contains a custom retro-computer presentation layer built with SDL2 2D rendering. The emulator content can be rendered into a recessed CRT-style display while the surrounding machine supplies the physical keyboard/control-panel appearance.

## Core CHIP-8 System

The emulator is built around the following CHIP-8 components:
- **Memory:** 4 KB (4096 bytes)
- **Program start:** 0x200
- **Registers:** 16 general-purpose 8-bit registers (V0–VF)
- **Index register:** 16-bit I
- **Program counter:** 16-bit PC
- **Stack:** 16 levels with stack pointer (SP)
- **Display:** 64 × 32 monochrome pixels
- **Timers:** delay timer and sound timer, updated at 60 Hz
- **Keypad:** 16-key hexadecimal keypad
- **Instruction set:** all 35 standard CHIP-8 instructions are implemented in the current project sources

## Features

### CHIP-8 Emulation
- CHIP-8 opcode execution
- Memory and register management
- Stack and subroutine handling
- 64 × 32 monochrome display
- Keyboard input
- Delay and sound timers
- SDL2-based audio beep
- SDL2-based rendering

### ROMs Included
The repository currently includes example ROMs:
```
roms/
├── Blinky.ch8
├── Pong.ch8
└── Tetris.ch8
```

### Save States
The project contains emulator save/load support and per-ROM save files under:
```
saves/
├── Blinky.ch8.sav
├── Pong.ch8.sav
└── Tetris.ch8.sav
```
The emulator state serialization code stores/restores the complete emulator state, including the 4 KB memory, registers, index register, program counter, stack, stack pointer, timers, and display state.

### Runtime Speed Control
The current project sources include a runtime `cycles_per_frame` value and keyboard-based speed adjustment logic. The speed can be increased or decreased during execution.

### RetroComputerUI
The project includes a custom `RetroComputerUI` implemented in C++/SDL2. Its purpose is to present the emulator inside a retro-computer shell rather than as a plain CHIP-8 window.

The UI implementation includes:
- 2.5D / pixel-art retro-computer rendering
- Layered case faces and depth
- Recessed CRT bezel and glass
- CRT scanlines and subtle display effects
- Virtual CHIP-8 keypad
- Physical-style controls
- Palette definitions
- Pixel-font text rendering
- Screen/content rectangle exposed for emulator integration
- Virtual-key press/release callbacks

The 3D appearance is produced with SDL2 2D primitives such as rectangles, polygons, offset shadows and bevel highlights; no real 3D engine is required.

## Retro-Computer Rendering Architecture

The intended rendering flow is:
```
CHIP-8 ROM
   ↓
CHIP-8 CPU / emulator state
   ↓
CHIP-8 framebuffer or existing emulator UI
   ↓
CRT content area
   ↓
RetroComputerUI
   ↓
SDL2 window
```
The GUI exposes the CRT content rectangle through:
`SDL_Rect getScreenArea() const;`
and provides a frame pipeline based on:
`ui.beginFrame(); ui.renderContent(contentTexture); ui.endFrame();`

The supplied standalone GUI package in the archive documents this integration pattern in its `INTEGRATION.md` file.

## Keyboard Mapping

The standard CHIP-8 keypad is mapped to a QWERTY keyboard in the following layout:

```
CHIP-8:             QWERTY:
┌─┬─┬─┬─┐           ┌─┬─┬─┬─┐
│1│2│3│C│           │1│2│3│4│
├─┼─┼─┼─┤           ├─┼─┼─┼─┤
│4│5│6│D│     =     │Q│W│E│R│
├─┼─┼─┼─┤           ├─┼─┼─┼─┤
│7│8│9│E│           │A│S│D│F│
├─┼─┼─┼─┤           ├─┼─┼─┼─┤
│A│0│B│F│           │Z│X│C│V│
└─┴─┴─┴─┘           └─┴─┴─┴─┘
```

The current source also contains additional directional/space mappings for the virtual/extended controls:
- UP → CHIP-8 key 2
- DOWN → CHIP-8 key 8
- LEFT → CHIP-8 key 4
- RIGHT → CHIP-8 key 6
- SPACE → CHIP-8 key 5

These mappings are visible in the current source tree. Check the active merge state before relying on them in a final build.

## Emulator Controls

- `ESC` → exit / pause depending on the active control flow
- `F5` / `S` → quick save
- `F9` / `L` → quick load
- `+` / `=` → increase emulation speed
- `-` → decrease emulation speed

### Game-Specific Controls Currently Documented by the Project

**PONG**
- Left paddle: `1` = up, `Q` = down
- Right paddle: `4` = up, `R` = down

**TETRIS**
- `Q` = rotate
- `W` = drop
- `E` = move right
- `A` = move left

*(The archive does not document a separate game-specific control list for Blinky.)*

## Audio

The existing emulator contains SDL2 audio support for the CHIP-8 sound timer.
The documented/default sound behavior is a simple beep generated when:
`sound_timer > 0`
The current project sources use a simple square-wave style tone at approximately 440 Hz.

## Project Structure

A simplified view of the archive is:
```text
Paruppuvada/
│
├── CHIP-8/
│   ├── src/
│   │   ├── chip8.cpp
│   │   ├── chip8.h
│   │   ├── main.cpp
│   │   ├── retro_computer.cpp
│   │   └── retro_computer.h
│   │
│   ├── roms/
│   │   ├── Blinky.ch8
│   │   ├── Pong.ch8
│   │   └── Tetris.ch8
│   │
│   ├── saves/
│   │   ├── Blinky.ch8.sav
│   │   ├── Pong.ch8.sav
│   │   └── Tetris.ch8.sav
│   │
│   ├── Makefile
│   ├── README.md
│   ├── chip8.exe
│   └── SDL2.dll
│
├── Kimi_Agent_Retro Computer Menu Design/
│   └── RetroComputerGUI/
│       ├── src/
│       │   ├── retro_computer.cpp
│       │   ├── retro_computer.h
│       │   └── demo_main.cpp
│       ├── INTEGRATION.md
│       ├── Makefile
│       ├── preview_green.png
│       └── preview_amber.png
│
└── .vscode/
```

## RetroComputerUI API

The current GUI header exposes the following main integration concepts.

### Initialization
```cpp
RetroComputerUI ui;
ui.init(renderer, windowWidth, windowHeight);
```

### CRT Content
```cpp
SDL_Rect area = ui.getScreenArea();
```
This rectangle is the target area for the existing emulator content.

### Rendering
```cpp
ui.beginFrame();
ui.renderContent(contentTexture);
ui.endFrame();
```

### Virtual Keypad
```cpp
ui.setVirtualKeyPressed(key, pressed);
ui.handleEvent(event);
ui.setKeyCallback(callback, userdata);
```
The GUI itself is designed not to own CHIP-8 CPU state; the host emulator connects the virtual-key callback to the actual input array or input system.

### Palette
The current GUI package defines built-in retro palettes and supports custom palette definitions through its public API.

## Building on Windows with MSYS2

The provided Makefile targets GCC/MinGW and links SDL2.

**1. Install MSYS2**
Install MSYS2 from:
https://www.msys2.org/
Use the **MSYS2 UCRT64** terminal.

**2. Install Dependencies**
```bash
pacman -S --noconfirm mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-make mingw-w64-ucrt-x86_64-SDL2
```

**3. Build**
From the CHIP-8 directory:
```bash
mingw32-make
```

**4. Run a ROM**
```bash
./chip8 roms/Tetris.ch8
```
Other included ROMs:
```bash
./chip8 roms/Pong.ch8
./chip8 roms/Blinky.ch8
```

## Building the Standalone RetroComputerGUI Demo

The archive also contains a standalone GUI verification package under:
`Kimi_Agent_Retro Computer Menu Design/RetroComputerGUI/`

Its demo is intended to verify the GUI independently of the CHIP-8 core.
From that directory:
```bash
make demo
./gui_demo
```
The standalone demo uses placeholder CRT content and is not a replacement for the real emulator.

## Integration Notes

The intended integration strategy is to keep the emulator logic and GUI rendering responsibilities separate:

```text
EMULATOR RESPONSIBILITY ────────────────────────
 CPU
 Memory
 Registers
 Timers
 Stack
 ROM loading
 Game logic
 Framebuffer
 Audio state
      │
      │ integration APIs
      ▼
GUI RESPONSIBILITY ──────────────────
 Retro computer body
 CRT presentation
 Keypad presentation
 GUI controls
 Palette presentation
 Pixel-art effects
```
The supplied `Kimi_Agent_Retro Computer Menu Design/RetroComputerGUI/INTEGRATION.md` contains the detailed CRT texture/render-target approach and the public API usage examples.

## Development Notes

The archive currently contains source files that are in an unresolved Git merge-conflict state, including conflict markers such as:
`<<<<<<< HEAD ======= >>>>>>> ...`
These appear in the current `CHIP-8/src/chip8.h` and `CHIP-8/src/main.cpp` content in the uploaded archive. Resolve those conflicts before treating the archive as a clean buildable working tree.

The archive also contains compiled objects/executables (`.o`, `.exe`) and SDL2 runtime DLLs. For source control, it is generally preferable to keep generated build products out of the repository unless the project specifically requires them.

## Technology Stack
- C++17
- SDL2
- GCC / MinGW
- Make / MinGW Make
- CHIP-8 virtual machine architecture
- SDL2 audio
- SDL2 2D pixel-art rendering

## Credits / Resources
The project includes links in its existing documentation to the CHIP-8 ROM archive and SDL2/MSYS2 setup resources.

## License
The repository contains a `LICENSE` file under `CHIP-8/`. Refer to that file for the project's license terms.
