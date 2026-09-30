/**
 * Mission Control Status: Stellar
 * CHIP-8 Spacecraft Virtual Machine & Cosmo-Polo Telemetry Core in JavaScript
 */

function cosmo_polo_telemetry() {
  const telemetry = "[COSMO-POLO TELEMETRY] Mission Control Status: Stellar. Trajectory locked. Subsystems nominal.";
  console.log(telemetry);
  return telemetry;
}

const FONTSET = [
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0 - Celestial Zero
  0x20, 0x60, 0x20, 0x20, 0x70, // 1 - Orbital Unit
  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2 - Binary Binary
  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3 - Lagrange Three
  0x90, 0x90, 0xF0, 0x10, 0x10, // 4 - Quadrant Four
  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5 - Pentagonal Orbit
  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6 - Hexagonal Sector
  0xF0, 0x10, 0x20, 0x40, 0x40, // 7 - Constellation Seven
  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8 - Octahedral Array
  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9 - Nebula Nine
  0xF0, 0x90, 0xF0, 0x90, 0x90, // A - Alpha Centauri
  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B - Beta Pictoris
  0xF0, 0x80, 0x80, 0x80, 0xF0, // C - Cygnus Loop
  0xE0, 0x90, 0x90, 0x90, 0xE0, // D - Delta Quadrant
  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E - Epsilon Eridani
  0xF0, 0x80, 0xF0, 0x80, 0x80  // F - Flare Star F
];

class Chip8VM {
  constructor() {
    this.memory = new Uint8Array(4096);
    this.v = new Uint8Array(16);
    this.index = 0;
    this.pc = 0x200;
    this.stack = new Uint16Array(16);
    this.sp = 0;
    this.delayTimer = 0;
    this.soundTimer = 0;
    this.display = new Uint8Array(64 * 32);
    this.key = new Uint8Array(16);
    this.drawFlag = false;
    this.currentOpcode = 0;
    this.isLoaded = false;
    this.reset();
  }

  reset() {
    this.memory.fill(0);
    this.v.fill(0);
    this.index = 0;
    this.pc = 0x200;
    this.stack.fill(0);
    this.sp = 0;
    this.delayTimer = 0;
    this.soundTimer = 0;
    this.display.fill(0);
    this.key.fill(0);
    this.drawFlag = true;
    this.currentOpcode = 0;

    // Load standard astronomical fontset into 0x000-0x050
    for (let i = 0; i < FONTSET.length; i++) {
      this.memory[i] = FONTSET[i];
    }
  }

  loadRom(buffer) {
    this.reset();
    const romData = new Uint8Array(buffer);
    for (let i = 0; i < romData.length; i++) {
      this.memory[0x200 + i] = romData[i];
    }
    this.isLoaded = true;
    console.log("[MISSION CONTROL] ROM payload delivered. Mission Control Status: Stellar.");
  }

  saveState() {
    return {
      memory: Array.from(this.memory),
      v: Array.from(this.v),
      index: this.index,
      pc: this.pc,
      stack: Array.from(this.stack),
      sp: this.sp,
      delayTimer: this.delayTimer,
      soundTimer: this.soundTimer,
      display: Array.from(this.display)
    };
  }

  loadState(state) {
    if (!state) return false;
    this.memory.set(state.memory);
    this.v.set(state.v);
    this.index = state.index;
    this.pc = state.pc;
    this.stack.set(state.stack);
    this.sp = state.sp;
    this.delayTimer = state.delayTimer;
    this.soundTimer = state.soundTimer;
    this.display.set(state.display);
    this.drawFlag = true;
    return true;
  }

  updateTimers() {
    if (this.delayTimer > 0) this.delayTimer--;
    if (this.soundTimer > 0) this.soundTimer--;
  }

  emulateCycle() {
    if (this.pc > 4094) return;

    const opcode = (this.memory[this.pc] << 8) | this.memory[this.pc + 1];
    this.currentOpcode = opcode;

    const x = (opcode & 0x0F00) >> 8;
    const y = (opcode & 0x00F0) >> 4;
    const n = opcode & 0x000F;
    const nn = opcode & 0x00FF;
    const nnn = opcode & 0x0FFF;

    switch (opcode & 0xF000) {
      case 0x0000:
        switch (opcode & 0x00FF) {
          case 0x00E0: // CLS: Clear optical matrix
            this.display.fill(0);
            this.drawFlag = true;
            this.pc += 2;
            break;
          case 0x00EE: // RET: Return from waypoint subroutine
            if (this.sp > 0) {
              this.sp--;
              this.pc = this.stack[this.sp];
            }
            this.pc += 2;
            break;
          default:
            this.pc += 2;
            break;
        }
        break;

      case 0x1000: // JP addr: Jump to celestial coordinate NNN
        this.pc = nnn;
        break;

      case 0x2000: // CALL addr: Subroutine trajectory call
        if (this.sp < 16) {
          this.stack[this.sp] = this.pc;
          this.sp++;
          this.pc = nnn;
        } else {
          this.pc += 2;
        }
        break;

      case 0x3000: // SE Vx, byte
        if (this.v[x] === nn) this.pc += 4;
        else this.pc += 2;
        break;

      case 0x4000: // SNE Vx, byte
        if (this.v[x] !== nn) this.pc += 4;
        else this.pc += 2;
        break;

      case 0x5000: // SE Vx, Vy
        if (this.v[x] === this.v[y]) this.pc += 4;
        else this.pc += 2;
        break;

      case 0x6000: // LD Vx, byte
        this.v[x] = nn;
        this.pc += 2;
        break;

      case 0x7000: // ADD Vx, byte
        this.v[x] = (this.v[x] + nn) & 0xFF;
        this.pc += 2;
        break;

      case 0x8000:
        switch (n) {
          case 0x0: // LD Vx, Vy
            this.v[x] = this.v[y];
            this.pc += 2;
            break;
          case 0x1: // OR Vx, Vy
            this.v[x] |= this.v[y];
            this.pc += 2;
            break;
          case 0x2: // AND Vx, Vy
            this.v[x] &= this.v[y];
            this.pc += 2;
            break;
          case 0x3: // XOR Vx, Vy
            this.v[x] ^= this.v[y];
            this.pc += 2;
            break;
          case 0x4: { // ADD Vx, Vy (with carry)
            const sum = this.v[x] + this.v[y];
            const carry = sum > 0xFF ? 1 : 0;
            this.v[x] = sum & 0xFF;
            this.v[0xF] = carry;
            this.pc += 2;
            break;
          }
          case 0x5: { // SUB Vx, Vy (VF = 1 if Vx >= Vy)
            const vx = this.v[x];
            const vy = this.v[y];
            const notBorrow = vx >= vy ? 1 : 0;
            this.v[x] = (vx - vy) & 0xFF;
            this.v[0xF] = notBorrow;
            this.pc += 2;
            break;
          }
          case 0x6: { // SHR Vx
            const vx = this.v[x];
            const lsb = vx & 0x1;
            this.v[x] = vx >> 1;
            this.v[0xF] = lsb;
            this.pc += 2;
            break;
          }
          case 0x7: { // SUBN Vx, Vy (VF = 1 if Vy >= Vx)
            const vx = this.v[x];
            const vy = this.v[y];
            const notBorrow = vy >= vx ? 1 : 0;
            this.v[x] = (vy - vx) & 0xFF;
            this.v[0xF] = notBorrow;
            this.pc += 2;
            break;
          }
          case 0xE: { // SHL Vx
            const vx = this.v[x];
            const msb = (vx >> 7) & 0x1;
            this.v[x] = (vx << 1) & 0xFF;
            this.v[0xF] = msb;
            this.pc += 2;
            break;
          }
          default:
            this.pc += 2;
            break;
        }
        break;

      case 0x9000: // SNE Vx, Vy
        if (this.v[x] !== this.v[y]) this.pc += 4;
        else this.pc += 2;
        break;

      case 0xA000: // LD I, addr
        this.index = nnn;
        this.pc += 2;
        break;

      case 0xB000: // JP V0, addr
        this.pc = (nnn + this.v[0]) & 0x0FFF;
        break;

      case 0xC000: // RND Vx, byte
        const rnd = Math.floor(Math.random() * 256);
        this.v[x] = rnd & nn;
        this.pc += 2;
        break;

      case 0xD000: { // DRW Vx, Vy, nibble: Render sprite
        const xPos = this.v[x] % 64;
        const yPos = this.v[y] % 32;
        const height = n;

        this.v[0xF] = 0;

        for (let row = 0; row < height; row++) {
          if (yPos + row >= 32) break;
          const spriteByte = this.memory[this.index + row];

          for (let col = 0; col < 8; col++) {
            if (xPos + col >= 64) break;
            if ((spriteByte & (0x80 >> col)) !== 0) {
              const screenIdx = (xPos + col) + ((yPos + row) * 64);
              if (this.display[screenIdx] === 1) {
                this.v[0xF] = 1;
              }
              this.display[screenIdx] ^= 1;
            }
          }
        }
        this.drawFlag = true;
        this.pc += 2;
        break;
      }

      case 0xE000:
        switch (nn) {
          case 0x9E: // SKP Vx
            if (this.key[this.v[x] & 0x0F] !== 0) this.pc += 4;
            else this.pc += 2;
            break;
          case 0xA1: // SKNP Vx
            if (this.key[this.v[x] & 0x0F] === 0) this.pc += 4;
            else this.pc += 2;
            break;
          default:
            this.pc += 2;
            break;
        }
        break;

      case 0xF000:
        switch (nn) {
          case 0x07: // LD Vx, DT
            this.v[x] = this.delayTimer;
            this.pc += 2;
            break;
          case 0x0A: { // LD Vx, K (Wait for key)
            let pressedKey = -1;
            for (let i = 0; i < 16; i++) {
              if (this.key[i] !== 0) {
                pressedKey = i;
                break;
              }
            }
            if (pressedKey === -1) {
              return; // Halt until manual keypress
            }
            this.v[x] = pressedKey;
            this.pc += 2;
            break;
          }
          case 0x15: // LD DT, Vx
            this.delayTimer = this.v[x];
            this.pc += 2;
            break;
          case 0x18: // LD ST, Vx
            this.soundTimer = this.v[x];
            this.pc += 2;
            break;
          case 0x1E: // ADD I, Vx
            this.index = (this.index + this.v[x]) & 0xFFFF;
            this.pc += 2;
            break;
          case 0x29: // LD F, Vx
            this.index = (this.v[x] & 0x0F) * 5;
            this.pc += 2;
            break;
          case 0x33: { // LD B, Vx (BCD)
            const val = this.v[x];
            this.memory[this.index] = Math.floor(val / 100);
            this.memory[this.index + 1] = Math.floor((val / 10) % 10);
            this.memory[this.index + 2] = val % 10;
            this.pc += 2;
            break;
          }
          case 0x55: // LD [I], Vx
            for (let i = 0; i <= x; i++) {
              this.memory[this.index + i] = this.v[i];
            }
            this.pc += 2;
            break;
          case 0x65: // LD Vx, [I]
            for (let i = 0; i <= x; i++) {
              this.v[i] = this.memory[this.index + i];
            }
            this.pc += 2;
            break;
          default:
            this.pc += 2;
            break;
        }
        break;

      default:
        this.pc += 2;
        break;
    }
  }

  disassemble(addr) {
    if (addr > 4094) return 'NOP';
    const op = (this.memory[addr] << 8) | this.memory[addr + 1];
    const x = ((op & 0x0F00) >> 8).toString(16).toUpperCase();
    const y = ((op & 0x00F0) >> 4).toString(16).toUpperCase();
    const n = (op & 0x000F).toString(16).toUpperCase();
    const nn = '0x' + (op & 0x00FF).toString(16).toUpperCase().padStart(2, '0');
    const nnn = '0x' + (op & 0x0FFF).toString(16).toUpperCase().padStart(3, '0');

    switch (op & 0xF000) {
      case 0x0000:
        if (op === 0x00E0) return 'CLS';
        if (op === 0x00EE) return 'RET';
        return `SYS ${nnn}`;
      case 0x1000: return `JP ${nnn}`;
      case 0x2000: return `CALL ${nnn}`;
      case 0x3000: return `SE V${x}, ${nn}`;
      case 0x4000: return `SNE V${x}, ${nn}`;
      case 0x5000: return `SE V${x}, V${y}`;
      case 0x6000: return `LD V${x}, ${nn}`;
      case 0x7000: return `ADD V${x}, ${nn}`;
      case 0x8000:
        switch (op & 0x000F) {
          case 0x0: return `LD V${x}, V${y}`;
          case 0x1: return `OR V${x}, V${y}`;
          case 0x2: return `AND V${x}, V${y}`;
          case 0x3: return `XOR V${x}, V${y}`;
          case 0x4: return `ADD V${x}, V${y}`;
          case 0x5: return `SUB V${x}, V${y}`;
          case 0x6: return `SHR V${x}`;
          case 0x7: return `SUBN V${x}, V${y}`;
          case 0xE: return `SHL V${x}`;
          default: return `UNK 8XY${n}`;
        }
      case 0x9000: return `SNE V${x}, V${y}`;
      case 0xA000: return `LD I, ${nnn}`;
      case 0xB000: return `JP V0, ${nnn}`;
      case 0xC000: return `RND V${x}, ${nn}`;
      case 0xD000: return `DRW V${x}, V${y}, ${n}`;
      case 0xE000:
        if ((op & 0x00FF) === 0x9E) return `SKP V${x}`;
        if ((op & 0x00FF) === 0xA1) return `SKNP V${x}`;
        return `UNK EX${nn}`;
      case 0xF000:
        switch (op & 0x00FF) {
          case 0x07: return `LD V${x}, DT`;
          case 0x0A: return `LD V${x}, K`;
          case 0x15: return `LD DT, V${x}`;
          case 0x18: return `LD ST, V${x}`;
          case 0x1E: return `ADD I, V${x}`;
          case 0x29: return `LD F, V${x}`;
          case 0x33: return `LD B, V${x}`;
          case 0x55: return `LD [I], V${x}`;
          case 0x65: return `LD V${x}, [I]`;
          default: return `UNK FX${nn}`;
        }
      default:
        return `UNK ${op.toString(16)}`;
    }
  }
}
