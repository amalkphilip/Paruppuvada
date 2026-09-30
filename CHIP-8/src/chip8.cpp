#include "chip8.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <cstring>
#include <random>

uint8_t chip8_fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

Chip8::Chip8(){
    initialise();
}

void Chip8::initialise(){
    pc = 0x200;
    opcode = 0;
    index = 0;
    sp = 0;

    memset(display, 0, sizeof(display));
    memset(stack, 0, sizeof(stack));
    memset(v, 0, sizeof(v));
    memset(memory, 0, sizeof(memory));
    memset(key, 0, sizeof(key));

    load_fonts();
    delay_timer = 0;
    sound_timer = 0;
    draw_flag = false;
}

void Chip8::load_fonts(){
    for(int i=0; i<80; i++) memory[i] = chip8_fontset[i];
}

void Chip8::load_rom(const std::string& filename){
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if(!file.is_open()){
        std::cerr << "Failed to open ROM: " << filename << std::endl;
        return;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if(size > (4096-512)){ // 512 reserved for fonts/interpreter
        std::cerr << "ROM too large to fit in memory" << std::endl;
        return;
    }

    file.read((char*)(memory+512), size);
    file.close();

    std::cout << "Loaded ROM: " << filename << " (" << size << " bytes)" << std::endl;
}

void Chip8::update_timers(){
    if(delay_timer > 0) delay_timer--;
    if(sound_timer > 0) sound_timer--;
}

void Chip8::emulate_cycle(){
    if(pc > 4094) {
        std::cerr << "Program counter out of bounds: 0x" << std::hex << pc << std::endl;
        return;
    }

    opcode = (memory[pc] << 8) | memory[pc+1]; // 16-bit instruction

    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;
    uint8_t n = opcode & 0x000F;
    uint8_t nn = opcode & 0x00FF;
    uint16_t nnn = opcode & 0x0FFF;

    switch(opcode & 0xF000){ // First nibble
        case 0x0000:
            switch(opcode & 0x00FF){
                case 0x00E0: // 00E0: Clear the display
                    std::memset(display, 0, sizeof(display));
                    draw_flag = true;
                    pc += 2;
                    break;
                case 0x00EE: // 00EE: Return from subroutine
                    if(sp > 0){
                        sp--;
                        pc = stack[sp];
                    }
                    pc += 2;
                    break;
                default:
                    std::cerr << "Unknown opcode: 0x" << std::hex << opcode << std::endl;
                    pc += 2;
                    break;
            }
            break;
        case 0x1000: // 1NNN: Jump to address NNN
            pc = nnn;
            break;
        case 0x2000: // 2NNN: Call subroutine at NNN
            if(sp < 16){
                stack[sp] = pc;
                sp++;
                pc = nnn;
            } else {
                std::cerr << "Stack overflow!" << std::endl;
                pc += 2;
            }
            break;
        case 0x3000: // 3XNN: Skip next instruction if V[X] == NN
            if(v[x] == nn) pc += 4;
            else pc += 2;
            break;
        case 0x4000: // 4XNN: Skip next instruction if V[X] != NN
            if(v[x] != nn) pc += 4;
            else pc += 2;
            break;
        case 0x5000: // 5XY0: Skip next instruction if V[X] == V[Y]
            if((opcode & 0x000F) == 0){
                if(v[x] == v[y]) pc += 4;
                else pc += 2;
            } else {
                std::cerr << "Unknown opcode: 0x" << std::hex << opcode << std::endl;
                pc += 2;
            }
            break;
        case 0x6000: // 6XNN: Set V[X] = NN
            v[x] = nn;
            pc += 2;
            break;
        case 0x7000: // 7XNN: Set V[X] = V[X] + NN (VF not affected)
            v[x] += nn;
            pc += 2;
            break;
        case 0x8000: // 8XYN: Arithmetic and logic operations
            switch(n){
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
                case 0x4:{ // 8XY4: Set V[X] = V[X] + V[Y], VF = carry
                    uint16_t sum = (uint16_t)v[x] + (uint16_t)v[y];
                    uint8_t carry = (sum > 0xFF) ? 1 : 0;
                    v[x] = sum & 0xFF;
                    v[0xF] = carry;
                    pc += 2;
                    break;
                }
                case 0x5:{ // 8XY5: Set V[X] = V[X] - V[Y], VF = NOT borrow (1 if Vx >= Vy)
                    uint8_t vx = v[x];
                    uint8_t vy = v[y];
                    uint8_t not_borrow = (vx >= vy) ? 1 : 0;
                    v[x] = vx - vy;
                    v[0xF] = not_borrow;
                    pc += 2;
                    break;
                }
                case 0x6:{ // 8XY6: Set V[X] = V[X] SHR 1, VF = least significant bit
                    uint8_t vx = v[x];
                    uint8_t lsb = vx & 0x1;
                    v[x] = vx >> 1;
                    v[0xF] = lsb;
                    pc += 2;
                    break;
                }
                case 0x7:{ // 8XY7: Set V[X] = V[Y] - V[X], VF = NOT borrow (1 if Vy >= Vx)
                    uint8_t vx = v[x];
                    uint8_t vy = v[y];
                    uint8_t not_borrow = (vy >= vx) ? 1 : 0;
                    v[x] = vy - vx;
                    v[0xF] = not_borrow;
                    pc += 2;
                    break;
                }
                case 0xE:{ // 8XYE: Set V[X] = V[X] SHL 1, VF = most significant bit
                    uint8_t vx = v[x];
                    uint8_t msb = (vx >> 7) & 0x1;
                    v[x] = (vx << 1) & 0xFF;
                    v[0xF] = msb;
                    pc += 2;
                    break;
                }
                default:
                    std::cerr << "Unknown opcode: 0x" << std::hex << opcode << std::endl;
                    pc += 2;
                    break;
            }
            break;
        case 0x9000: // 9XY0: Skip next instruction if V[X] != V[Y]
            if((opcode & 0x000F) == 0){
                if(v[x] != v[y]) pc += 4;
                else pc += 2;
            } else {
                std::cerr << "Unknown opcode: 0x" << std::hex << opcode << std::endl;
                pc += 2;
            }
            break;
        case 0xA000: // ANNN: Set Index Register I = NNN
            index = nnn;
            pc += 2;
            break;
        case 0xB000: // BNNN: Jump to address NNN + V[0]
            pc = (nnn + v[0]) & 0x0FFF;
            break;
        case 0xC000:{ // CXNN: Set V[X] = random byte AND NN
            static std::random_device rd;
            static std::mt19937 gen(rd());
            static std::uniform_int_distribution<> dist(0, 255);
            v[x] = dist(gen) & nn;
            pc += 2;
            break;
        }
        case 0xD000:{ // DXYN: Draw sprite at coordinate (V[X], V[Y]) with height N
            uint8_t x_pos = v[x] % 64;
            uint8_t y_pos = v[y] % 32;
            uint8_t height = n;

            v[0xF] = 0; // Reset collision flag

            for(int row = 0; row < height; row++){
                if(y_pos + row >= 32) break; // Clip bottom
                uint8_t sprite_byte = memory[index + row];

                for(int col = 0; col < 8; col++){
                    if(x_pos + col >= 64) break; // Clip right
                    if((sprite_byte & (0x80 >> col)) != 0){
                        int screen_idx = (x_pos + col) + ((y_pos + row) * 64);
                        if(display[screen_idx] == 1){
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
            switch(nn){
                case 0x9E: // EX9E: Skip next instruction if key in V[X] is pressed
                    if(key[v[x] & 0x0F] != 0) pc += 4;
                    else pc += 2;
                    break;
                case 0xA1: // EXA1: Skip next instruction if key in V[X] is not pressed
                    if(key[v[x] & 0x0F] == 0) pc += 4;
                    else pc += 2;
                    break;
                default:
                    std::cerr << "Unknown opcode: 0x" << std::hex << opcode << std::endl;
                    pc += 2;
                    break;
            }
            break;
        case 0xF000: // Timers and memory operations
            switch(nn){
                case 0x07: // FX07: Set V[X] = delay_timer
                    v[x] = delay_timer;
                    pc += 2;
                    break;
                case 0x0A:{ // FX0A: Wait for key press and store in V[X]
                    bool key_pressed = false;
                    for(int i=0; i<16; i++){
                        if(key[i] != 0){
                            v[x] = i;
                            key_pressed = true;
                            break;
                        }
                    }
                    if(!key_pressed){
                        return; // Halt instruction execution until a key is pressed
                    }
                    pc += 2;
                    break;
                }
                case 0x15: // FX15: Set delay_timer = V[X]
                    delay_timer = v[x];
                    pc += 2;
                    break;
                case 0x18: // FX18: Set sound_timer = V[X]
                    sound_timer = v[x];
                    pc += 2;
                    break;
                case 0x1E: // FX1E: Set I = I + V[X]
                    index = (index + v[x]) & 0xFFFF;
                    pc += 2;
                    break;
                case 0x29: // FX29: Set I = location of sprite for digit V[X]
                    index = (v[x] & 0x0F) * 5;
                    pc += 2;
                    break;
                case 0x33:{ // FX33: Store BCD representation of V[X] at I, I+1, I+2
                    uint8_t value = v[x];
                    memory[index] = value / 100;
                    memory[index+1] = (value / 10) % 10;
                    memory[index+2] = value % 10;
                    pc += 2;
                    break;
                }
                case 0x55: // FX55: Store V0 to V[X] in memory starting at address I
                    for(int i=0; i<=x; i++){
                        memory[index+i] = v[i];
                    }
                    pc += 2;
                    break;
                case 0x65: // FX65: Load V0 to V[X] from memory starting at address I
                    for(int i=0; i<=x; i++){
                        v[i] = memory[index+i];
                    }
                    pc += 2;
                    break;
                default:
                    std::cerr << "Unknown opcode: 0x" << std::hex << opcode << std::endl;
                    pc += 2;
                    break;
            }
            break;
        default:
            std::cerr << "Unknown opcode: 0x" << std::hex << opcode << std::endl;
            pc += 2;
            break;
    }
}