# TatHack CHIP-8 Emulator: Comprehensive Defect Report & Solution

This document details all intentional implementation defects identified and resolved in the **TatHack CHIP-8 Emulator** repository.

---

## 🔍 Executive Summary of Defects

| Category | Defect Identified | Root Cause | Impact | Resolution |
|---|---|---|---|---|
| **Memory / Stack** | `0x00EE` (RET) stack underflow/misalignment | `pc = stack[sp]; sp--;` reads garbage slot before decrementing | Calling functions corrupts `PC` and crashes games | Decrement `sp` before reading `stack[sp]` |
| **Arithmetic (SUB)** | `0x8XY5` comparison using `>` instead of `>=` | `(Vx > Vy)` sets `VF = 0` when `Vx == Vy` | Arithmetic operations fail borrow test when values are equal | Used `(vx >= vy) ? 1 : 0` |
| **Arithmetic (SUBN)** | `0x8XY7` comparison using `>` instead of `>=` | `(Vy > Vx)` sets `VF = 0` when `Vy == Vx` | Same as above for reversed operand subtract | Used `(vy >= vx) ? 1 : 0` |
| **Input (Block/Wait)** | `0xFX0A` unconditional `PC` advancement | `pc += 2` executed even if no key was pressed | Skips blocking wait, corrupting input states | Halt and return without advancing `PC` until key pressed |
| **BCD Conversion** | `0xFX33` Tens digit calculation | `memory[index+1] = value / 10;` stores quotient instead of modulo 10 (e.g., 159 &rarr; 15 instead of 5) | Corrupts numeric scores & digits in games | Used `(value / 10) % 10` |
| **Memory Block R/W** | `0xFX55` & `0xFX65` loop bound error | `for (int i = 0; i < x; i++)` instead of `i <= x` | Misses saving/restoring register `Vx` | Changed loop condition to `i <= x` |
| **Graphics & Rendering** | Y-axis inverted in `main.cpp` | `SDL_Rect rect = {x*SCALE, (31-y)*SCALE, SCALE, SCALE};` | Inverts the screen upside down | Changed to `y*SCALE` |
| **Timing & Clock Control** | Timers decremented inside opcode cycle | `delay_timer` and `sound_timer` decremented per instruction (~500Hz) instead of 60Hz | Timers expire ~10x too fast, breaking game physics | Created `update_timers()` called once per 60Hz frame |
| **Frame Pacing** | `SDL_Delay(16)` inside instruction loop | `SDL_Delay(16)` executed 10 times per frame (total 160ms = ~6 FPS) | Severe lag and unplayable frame rates | Executed 9 cycles per frame then delayed frame target once |
