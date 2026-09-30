#include "chip8.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <random>

// Mission Control Status: Stellar
// Cosmo-Polo Space Exploration & Orbital Execution Telemetry
void cosmo_polo_telemetry() {
    std::cout << "==================================================" << std::endl;
    std::cout << "[COSMO-POLO TELEMETRY] Mission Control Status: Stellar" << std::endl;
    std::cout << "[COSMO-POLO TELEMETRY] Navigational trajectory stable." << std::endl;
    std::cout << "[COSMO-POLO TELEMETRY] Subsystems operational at optimal cadence." << std::endl;
    std::cout << "==================================================" << std::endl;
}

// Standard Astronomical Hexadecimal Font Glyphs (0-F)
uint8_t chip8_fontset[80] = {
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
};

Chip8::Chip8() {
    initialise();
}

void Chip8::initialise() {
    // Mission Launch Coordinates: Program Counter initialized to 0x200
    pc = 0x200;
    opcode = 0;
    index = 0;
    sp = 0;

    std::memset(display, 0, sizeof(display));
    std::memset(stack, 0, sizeof(stack));
    std::memset(v, 0, sizeof(v));
    std::memset(memory, 0, sizeof(memory));
    std::memset(key, 0, sizeof(key));

    load_fonts();
    delay_timer = 0;
    sound_timer = 0;
    draw_flag = false;
}

void Chip8::load_fonts() {
    // Storing astronomical font glyphs in reserved low memory (0x000 - 0x050)
    for (int i = 0; i < 80; i++) {
        memory[i] = chip8_fontset[i];
    }
}

void Chip8::load_rom(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        std::cerr << "[MISSION CONTROL] Failed to access orbital payload: " << filename << std::endl;
        return;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size > (4096 - 512)) {
        std::cerr << "[MISSION CONTROL] ROM payload exceeds spacecraft memory envelope!" << std::endl;
        return;
    }

    // Deploy payload into orbital memory space starting at 0x200
    file.read(reinterpret_cast<char*>(memory + 512), size);
    file.close();

    std::cout << "[MISSION CONTROL] Orbital ROM payload delivered: " << filename
              << " (" << size << " bytes). Mission Control Status: Stellar." << std::endl;
}

void Chip8::update_timers() {
    // Spacecraft Chronometer Synchronization at 60 Hz
    if (delay_timer > 0) delay_timer--;
    if (sound_timer > 0) sound_timer--;
}

bool Chip8::save_state(const std::string& filename) {
    // Capture celestial coordinates and snapshot full spacecraft state
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[MISSION CONTROL] Failed to write savestate to " << filename << std::endl;
        return false;
    }

    file.write(reinterpret_cast<const char*>(memory), sizeof(memory));
    file.write(reinterpret_cast<const char*>(v), sizeof(v));
    file.write(reinterpret_cast<const char*>(&index), sizeof(index));
    file.write(reinterpret_cast<const char*>(&pc), sizeof(pc));
    file.write(reinterpret_cast<const char*>(stack), sizeof(stack));
    file.write(reinterpret_cast<const char*>(&sp), sizeof(sp));
    file.write(reinterpret_cast<const char*>(&delay_timer), sizeof(delay_timer));
    file.write(reinterpret_cast<const char*>(&sound_timer), sizeof(sound_timer));
    file.write(reinterpret_cast<const char*>(display), sizeof(display));
    file.close();

    std::cout << "[SAVESTATE] Celestial coordinates successfully saved to " << filename << std::endl;
    return true;
}

bool Chip8::load_state(const std::string& filename) {
    // Restore flight parameters and orbital position from disk
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[MISSION CONTROL] Savestate file not found: " << filename << std::endl;
        return false;
    }

    file.read(reinterpret_cast<char*>(memory), sizeof(memory));
    file.read(reinterpret_cast<char*>(v), sizeof(v));
    file.read(reinterpret_cast<char*>(&index), sizeof(index));
    file.read(reinterpret_cast<char*>(&pc), sizeof(pc));
    file.read(reinterpret_cast<char*>(stack), sizeof(stack));
    file.read(reinterpret_cast<char*>(&sp), sizeof(sp));
    file.read(reinterpret_cast<char*>(&delay_timer), sizeof(delay_timer));
    file.read(reinterpret_cast<char*>(&sound_timer), sizeof(sound_timer));
    file.read(reinterpret_cast<char*>(display), sizeof(display));
    file.close();

    draw_flag = true;
    std::cout << "[SAVESTATE] Celestial coordinates successfully restored from " << filename << std::endl;
    return true;
}

std::string Chip8::disassemble_current_opcode() const {
    if (pc > 4094) return "NOP (Out of Bounds)";
    uint16_t op = (memory[pc] << 8) | memory[pc + 1];

    std::stringstream ss;
    ss << "0x" << std::uppercase << std::hex << std::setw(4) << std::setfill('0') << op << " : ";

    uint8_t x = (op & 0x0F00) >> 8;
    uint8_t y = (op & 0x00F0) >> 4;
    uint8_t n = op & 0x000F;
    uint8_t nn = op & 0x00FF;
    uint16_t nnn = op & 0x0FFF;

    switch (op & 0xF000) {
        case 0x0000:
            if (op == 0x00E0) ss << "CLS (Clear Sensor Grid)";
            else if (op == 0x00EE) ss << "RET (Return from Subroutine)";
            else ss << "SYS 0x" << std::hex << nnn;
            break;
        case 0x1000: ss << "JP 0x" << std::hex << nnn; break;
        case 0x2000: ss << "CALL 0x" << std::hex << nnn; break;
        case 0x3000: ss << "SE V" << (int)x << ", 0x" << std::hex << (int)nn; break;
        case 0x4000: ss << "SNE V" << (int)x << ", 0x" << std::hex << (int)nn; break;
        case 0x5000: ss << "SE V" << (int)x << ", V" << (int)y; break;
        case 0x6000: ss << "LD V" << (int)x << ", 0x" << std::hex << (int)nn; break;
        case 0x7000: ss << "ADD V" << (int)x << ", 0x" << std::hex << (int)nn; break;
        case 0x8000:
            switch (n) {
                case 0x0: ss << "LD V" << (int)x << ", V" << (int)y; break;
                case 0x1: ss << "OR V" << (int)x << ", V" << (int)y; break;
                case 0x2: ss << "AND V" << (int)x << ", V" << (int)y; break;
                case 0x3: ss << "XOR V" << (int)x << ", V" << (int)y; break;
                case 0x4: ss << "ADD V" << (int)x << ", V" << (int)y << " (Carry)"; break;
                case 0x5: ss << "SUB V" << (int)x << ", V" << (int)y << " (Borrow)"; break;
                case 0x6: ss << "SHR V" << (int)x; break;
                case 0x7: ss << "SUBN V" << (int)x << ", V" << (int)y; break;
                case 0xE: ss << "SHL V" << (int)x; break;
                default: ss << "UNK 0x" << std::hex << op; break;
            }
            break;
        case 0x9000: ss << "SNE V" << (int)x << ", V" << (int)y; break;
        case 0xA000: ss << "LD I, 0x" << std::hex << nnn; break;
        case 0xB000: ss << "JP V0, 0x" << std::hex << nnn; break;
        case 0xC000: ss << "RND V" << (int)x << ", 0x" << std::hex << (int)nn; break;
        case 0xD000: ss << "DRW V" << (int)x << ", V" << (int)y << ", " << (int)n; break;
        case 0xE000:
            if (nn == 0x9E) ss << "SKP V" << (int)x;
            else if (nn == 0xA1) ss << "SKNP V" << (int)x;
            else ss << "UNK 0x" << std::hex << op;
            break;
        case 0xF000:
            switch (nn) {
                case 0x07: ss << "LD V" << (int)x << ", DT"; break;
                case 0x0A: ss << "LD V" << (int)x << ", K (Wait Key)"; break;
                case 0x15: ss << "LD DT, V" << (int)x; break;
                case 0x18: ss << "LD ST, V" << (int)x; break;
                case 0x1E: ss << "ADD I, V" << (int)x; break;
                case 0x29: ss << "LD F, V" << (int)x; break;
                case 0x33: ss << "LD B, V" << (int)x << " (BCD)"; break;
                case 0x55: ss << "LD [I], V" << (int)x; break;
                case 0x65: ss << "LD V" << (int)x << ", [I]"; break;
                default: ss << "UNK 0x" << std::hex << op; break;
            }
            break;
        default:
            ss << "UNK 0x" << std::hex << op;
            break;
    }
    return ss.str();
}

void Chip8::emulate_cycle() {
    if (pc > 4094) {
        std::cerr << "[MISSION CONTROL] Flight path out of bounds: 0x" << std::hex << pc << std::endl;
        return;
    }

    // Fetch 16-bit orbital instruction from memory bank
    opcode = (memory[pc] << 8) | memory[pc + 1];

    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;
    uint8_t n = opcode & 0x000F;
    uint8_t nn = opcode & 0x00FF;
    uint16_t nnn = opcode & 0x0FFF;

    // Decode & execute via propulsion logic unit
    switch (opcode & 0xF000) {
        case 0x0000:
            switch (opcode & 0x00FF) {
                case 0x00E0: // 00E0: Clear optical display grid
                    std::memset(display, 0, sizeof(display));
                    draw_flag = true;
                    pc += 2;
                    break;
                case 0x00EE: // 00EE: Return from waypoint subroutine
                    if (sp > 0) {
                        sp--;
                        pc = stack[sp];
                    }
                    pc += 2;
                    break;
                default:
                    std::cerr << "[MISSION CONTROL] Unknown opcode: 0x" << std::hex << opcode << std::endl;
                    pc += 2;
                    break;
            }
            break;

        case 0x1000: // 1NNN: Jump to orbital address NNN
            pc = nnn;
            break;

        case 0x2000: // 2NNN: Subroutine trajectory call to NNN
            if (sp < 16) {
                stack[sp] = pc;
                sp++;
                pc = nnn;
            } else {
                std::cerr << "[MISSION CONTROL] Navigation stack overflow!" << std::endl;
                pc += 2;
            }
            break;

        case 0x3000: // 3XNN: Skip next waypoint if V[X] == NN
            if (v[x] == nn) pc += 4;
            else pc += 2;
            break;

        case 0x4000: // 4XNN: Skip next waypoint if V[X] != NN
            if (v[x] != nn) pc += 4;
            else pc += 2;
            break;

        case 0x5000: // 5XY0: Skip next waypoint if V[X] == V[Y]
            if ((opcode & 0x000F) == 0) {
                if (v[x] == v[y]) pc += 4;
                else pc += 2;
            } else {
                std::cerr << "[MISSION CONTROL] Unknown opcode: 0x" << std::hex << opcode << std::endl;
                pc += 2;
            }
            break;

        case 0x6000: // 6XNN: Load constant NN into propulsion register V[X]
            v[x] = nn;
            pc += 2;
            break;

        case 0x7000: // 7XNN: Add constant NN to register V[X] (VF unaffected)
            v[x] += nn;
            pc += 2;
            break;

        case 0x8000: // 8XYN: Arithmetic & logic matrix operations
            switch (n) {
                case 0x0: // 8XY0: Set V[X] = V[Y]
                    v[x] = v[y];
                    pc += 2;
                    break;
                case 0x1: // 8XY1: Set V[X] = V[X] OR V[Y]
                    v[x] |= v[y];
                    pc += 2;
                    break;
                case 0x2: // 8XY2: Set V[X] = V[X] AND V[Y]
                    v[x] &= v[y];
                    pc += 2;
                    break;
                case 0x3: // 8XY3: Set V[X] = V[X] XOR V[Y]
                    v[x] ^= v[y];
                    pc += 2;
                    break;
                case 0x4: { // 8XY4: Set V[X] = V[X] + V[Y], VF = carry
                    uint16_t sum = static_cast<uint16_t>(v[x]) + static_cast<uint16_t>(v[y]);
                    uint8_t carry = (sum > 0xFF) ? 1 : 0;
                    v[x] = sum & 0xFF;
                    v[0xF] = carry;
                    pc += 2;
                    break;
                }
                case 0x5: { // 8XY5: Set V[X] = V[X] - V[Y], VF = NOT borrow (1 if Vx >= Vy)
                    uint8_t vx = v[x];
                    uint8_t vy = v[y];
                    uint8_t not_borrow = (vx >= vy) ? 1 : 0;
                    v[x] = vx - vy;
                    v[0xF] = not_borrow;
                    pc += 2;
                    break;
                }
                case 0x6: { // 8XY6: Set V[X] = V[X] >> 1, VF = LSB
                    uint8_t vx = v[x];
                    uint8_t lsb = vx & 0x1;
                    v[x] = vx >> 1;
                    v[0xF] = lsb;
                    pc += 2;
                    break;
                }
                case 0x7: { // 8XY7: Set V[X] = V[Y] - V[X], VF = NOT borrow (1 if Vy >= Vx)
                    uint8_t vx = v[x];
                    uint8_t vy = v[y];
                    uint8_t not_borrow = (vy >= vx) ? 1 : 0;
                    v[x] = vy - vx;
                    v[0xF] = not_borrow;
                    pc += 2;
                    break;
                }
                case 0xE: { // 8XYE: Set V[X] = V[X] << 1, VF = MSB
                    uint8_t vx = v[x];
                    uint8_t msb = (vx >> 7) & 0x1;
                    v[x] = (vx << 1) & 0xFF;
                    v[0xF] = msb;
                    pc += 2;
                    break;
                }
                default:
                    std::cerr << "[MISSION CONTROL] Unknown opcode: 0x" << std::hex << opcode << std::endl;
                    pc += 2;
                    break;
            }
            break;

        case 0x9000: // 9XY0: Skip next instruction if V[X] != V[Y]
            if ((opcode & 0x000F) == 0) {
                if (v[x] != v[y]) pc += 4;
                else pc += 2;
            } else {
                std::cerr << "[MISSION CONTROL] Unknown opcode: 0x" << std::hex << opcode << std::endl;
                pc += 2;
            }
            break;

        case 0xA000: // ANNN: Set Index Register I = NNN
            index = nnn;
            pc += 2;
            break;

        case 0xB000: // BNNN: Jump with offset V[0]
            pc = (nnn + v[0]) & 0x0FFF;
            break;

        case 0xC000: { // CXNN: Random entropy sampling AND NN
            static std::random_device rd;
            static std::mt19937 gen(rd());
            static std::uniform_int_distribution<> dist(0, 255);
            v[x] = dist(gen) & nn;
            pc += 2;
            break;
        }

        case 0xD000: { // DXYN: Render sprite on quantum matrix (V[X], V[Y]) height N
            uint8_t x_pos = v[x] % 64;
            uint8_t y_pos = v[y] % 32;
            uint8_t height = n;

            v[0xF] = 0; // Reset collision radar

            for (int row = 0; row < height; row++) {
                if (y_pos + row >= 32) break; // Sensor bounds clipping (bottom)
                uint8_t sprite_byte = memory[index + row];

                for (int col = 0; col < 8; col++) {
                    if (x_pos + col >= 64) break; // Sensor bounds clipping (right)
                    if ((sprite_byte & (0x80 >> col)) != 0) {
                        int screen_idx = (x_pos + col) + ((y_pos + row) * 64);
                        if (display[screen_idx] == 1) {
                            v[0xF] = 1; // Collision detected
                        }
                        display[screen_idx] ^= 1;
                    }
                }
            }
            draw_flag = true;
            pc += 2;
            break;
        }

        case 0xE000:
            switch (nn) {
                case 0x9E: // EX9E: Skip if keypad telemetry V[X] is active
                    if (key[v[x] & 0x0F] != 0) pc += 4;
                    else pc += 2;
                    break;
                case 0xA1: // EXA1: Skip if keypad telemetry V[X] is inactive
                    if (key[v[x] & 0x0F] == 0) pc += 4;
                    else pc += 2;
                    break;
                default:
                    std::cerr << "[MISSION CONTROL] Unknown opcode: 0x" << std::hex << opcode << std::endl;
                    pc += 2;
                    break;
            }
            break;

        case 0xF000: // Telemetry and memory transfers
            switch (nn) {
                case 0x07: // FX07: Set V[X] = Delay Chronometer
                    v[x] = delay_timer;
                    pc += 2;
                    break;

                case 0x0A: { // FX0A: Halt and await manual navigation keypress
                    bool key_pressed = false;
                    for (int i = 0; i < 16; i++) {
                        if (key[i] != 0) {
                            v[x] = i;
                            key_pressed = true;
                            break;
                        }
                    }
                    if (!key_pressed) {
                        return; // Halt trajectory execution until pilot input
                    }
                    pc += 2;
                    break;
                }

                case 0x15: // FX15: Set Delay Chronometer = V[X]
                    delay_timer = v[x];
                    pc += 2;
                    break;

                case 0x18: // FX18: Set Sound Beacon = V[X]
                    sound_timer = v[x];
                    pc += 2;
                    break;

                case 0x1E: // FX1E: Increment Index Register by V[X]
                    index = (index + v[x]) & 0xFFFF;
                    pc += 2;
                    break;

                case 0x29: // FX29: Set Index = glyph offset for digit V[X]
                    index = (v[x] & 0x0F) * 5;
                    pc += 2;
                    break;

                case 0x33: { // FX33: Binary-Coded Decimal encoding of V[X]
                    uint8_t value = v[x];
                    memory[index] = value / 100;
                    memory[index + 1] = (value / 10) % 10;
                    memory[index + 2] = value % 10;
                    pc += 2;
                    break;
                }

                case 0x55: // FX55: Store registers V[0] through V[X] into memory at I
                    for (int i = 0; i <= x; i++) {
                        memory[index + i] = v[i];
                    }
                    pc += 2;
                    break;

                case 0x65: // FX65: Restore registers V[0] through V[X] from memory at I
                    for (int i = 0; i <= x; i++) {
                        v[i] = memory[index + i];
                    }
                    pc += 2;
                    break;

                default:
                    std::cerr << "[MISSION CONTROL] Unknown opcode: 0x" << std::hex << opcode << std::endl;
                    pc += 2;
                    break;
            }
            break;

        default:
            std::cerr << "[MISSION CONTROL] Unknown opcode: 0x" << std::hex << opcode << std::endl;
            pc += 2;
            break;
    }
}
