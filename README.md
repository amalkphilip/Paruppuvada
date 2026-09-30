# Chip-8 Emulator

A Chip-8 emulator built in C++ with SDL2 graphics and audio support. 
All implementation defects (opcode logic, timing, memory, and rendering bugs) have been fixed!

## Features

- All 35 Chip-8 opcodes implemented correctly
- 64x32 pixel display with SDL2 rendering
- Keyboard input support
- Sound effects (beep tone)
- 60 FPS rendering

## Building and Running on Windows

To run this on Windows, you need a C++ compiler and the SDL2 library. The easiest way to get these is using MSYS2.

### 1. Install MSYS2
1. Download and install [MSYS2](https://www.msys2.org/). (Keep the default installation path `C:\msys64`).

### 2. Install the Toolchain (GCC, Make, SDL2)
1. Open the **MSYS2 UCRT64** terminal (Search for "MSYS2 UCRT64" in your Windows Start menu).
2. Run this command to install everything you need:
```bash
pacman -S --noconfirm mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-make mingw-w64-ucrt-x86_64-SDL2
```

### 3. Compile the Emulator
1. Still in the MSYS2 UCRT64 terminal, navigate to your repository folder. (Note: MSYS2 uses `/c/` or `/d/` instead of `C:\` or `D:\`):
```bash
cd /d/project/Thathva/Paruppuvada/CHIP-8
```
2. Build the project using the provided Makefile:
```bash
mingw32-make
```

### 4. Run a Game
Once compiled, run the executable and pass a ROM file to play!
```bash
./chip8 roms/Tetris.ch8
```
*(You can also try `Pong.ch8` or `Blinky.ch8`)*

---

## Keyboard Mapping

The original Chip-8 keypad is mapped to your QWERTY keyboard:
```
Chip-8 Keypad:          QWERTY Keyboard:
┌─┬─┬─┬─┐               ┌─┬─┬─┬─┐
│1│2│3│C│               │1│2│3│4│
├─┼─┼─┼─┤               ├─┼─┼─┼─┤
│4│5│6│D│               │Q│W│E│R│
├─┼─┼─┼─┤      =        ├─┼─┼─┼─┤
│7│8│9│E│               │A│S│D│F│
├─┼─┼─┼─┤               ├─┼─┼─┼─┤
│A│0│B│F│               │Z│X│C│V│
└─┴─┴─┴─┘               └─┴─┴─┴─┘
```

**Controls:**
- `ESC` - Quit emulator

### Game-Specific Controls

**PONG:**
- Left paddle: `1` (up), `Q` (down)
- Right paddle: `4` (up), `R` (down)

**TETRIS:**
- `Q` - Rotate
- `W` - Drop
- `E` - Move right
- `A` - Move left
