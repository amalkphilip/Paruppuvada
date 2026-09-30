# CHIP-8 Spacecraft Emulator & Telemetry Suite

A high-performance CHIP-8 virtual machine built in C++ with SDL2 graphics and audio support, accompanied by a real-time web emulator and live debugger.

> **Mission Control Status: Stellar**  
> All intentional defects across opcode execution, memory management, timing control, and rendering pipeline have been completely resolved and stabilized.

---

## 🚀 Key Features

- **All 35 CHIP-8 Opcodes Implemented & Verified**: Complete instruction set with exact flag/borrow behaviors.
- **64×32 Quantum Sensor Display**: Pixel-perfect rendering with collision detection and upright orientation.
- **60 Hz Spacecraft Chronometer Synchronization**: Delay and sound timers decremented at accurate 60 Hz cadence.
- **Configurable Emulation Warp Speed**: Dynamically increase/decrease CPU clock cycles per frame during runtime (`+` / `-`).
- **Savestate Management**: Full snapshot saving and restoring (`F5` to Save, `F6`/`F8` to Load).
- **5 Custom Celestial Color Palettes**: Toggleable in real-time via `TAB` (CRT Matrix Green, Amber Phosphor, Cyberpunk Neon, Deep Space Monochrome, Solar Gold).
- **Cosmo-Polo Telemetry Subsystem**: Space exploration telemetry uplink (`cosmo_polo_telemetry()`, shortcut `T`).
- **Live Disassembler & Debugger**: Real-time instruction decoding, register inspector (V0-VF, PC, I, SP, DT, ST), and stack view.
- **Interactive Web Interface**: Complete standalone web app in `web/` with embedded ROMs (`Pong`, `Tetris`, `Blinky`) and custom ROM drag-and-drop.

---

## 🕹️ Controls & Navigation

### Keyboard Mapping
```
CHIP-8 Keypad:          QWERTY Keyboard:
┌─┬─┬─┬─┐               ┌─┬─┬─┬─┐
│1│2│3│C│               │1│2│3│4│
├─┼─┼─┼─┤               ├─┼─┼─┼─┤
│4│5│6│D│       =       │Q│W│E│R│
├─┼─┼─┼─┤               ├─┼─┼─┼─┤
│7│8│9│E│               │A│S│D│F│
├─┼─┼─┼─┤               ├─┼─┼─┼─┤
│A│0│B│F│               │Z│X│C│V│
└─┴─┴─┴─┘               └─┴─┴─┴─┘
```

### Mission Control Shortcuts
| Key | Action |
|---|---|
| `ESC` | Abort mission / Quit emulator |
| `F5` | **Save State** (Write celestial state to `savestate.bin`) |
| `F6` / `F8` | **Load State** (Restore celestial state from `savestate.bin`) |
| `+` / `=` / `]` | **Increase Warp Speed** (+2 cycles/frame) |
| `-` / `_` / `[` | **Decrease Warp Speed** (-2 cycles/frame) |
| `TAB` / `F1` | **Cycle Color Palettes** (Green &rarr; Amber &rarr; Neon &rarr; Mono &rarr; Gold) |
| `P` / `SPACE` | **Pause / Resume** orbital execution |
| `O` / `N` | **Single-Step Instruction** (When paused, with disassembler log) |
| `T` | **Broadcast Cosmo-Polo Telemetry** |

---

## 🛠️ Building & Running (C++ Core)

### Prerequisites
- **Linux/Debian/Ubuntu**: `sudo apt-get install libsdl2-dev g++ make`
- **Arch Linux**: `sudo pacman -S sdl2 gcc make`
- **macOS**: `brew install sdl2 make`

### Build
```bash
make
```

### Launch ROM
```bash
./chip8 roms/Pong.ch8
# or
./chip8 roms/Tetris.ch8
# or
./chip8 roms/Blinky.ch8
```

---

## 🌐 Running Web Emulator & Live Debugger

Open `web/index.html` directly in your browser or run the local server:
```bash
node web/server.js
```
Then visit **[http://localhost:3000](http://localhost:3000)**.

---

## 🛰️ Cosmo-Polo Telemetry Report
For the complete technical breakdown of every bug fix and architectural correction, consult [DEFECT_REPORT.md](DEFECT_REPORT.md).