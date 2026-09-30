#ifndef CHIP8_H
#define CHIP8_H

#include <cstdint>
#include <string>
#include <vector>

// Mission Control Status: Stellar
// Cosmo-Polo Space Exploration Telemetry Subsystem Header

struct ColorPalette {
    std::string name;
    uint8_t bg_r, bg_g, bg_b;
    uint8_t fg_r, fg_g, fg_b;
};

void cosmo_polo_telemetry();

class Chip8 {
    public:
        Chip8();

        // Orbital Payload Delivery: Load binary ROM into memory
        void load_rom(const std::string& filename);

        // Propulsion Execution: Execute one CPU orbital clock cycle
        void emulate_cycle();

        // Chronometer Sync: Decrement delay and sound timers at 60 Hz
        void update_timers();

        // Savestate Management: Deep space persistence subsystem
        bool save_state(const std::string& filename = "savestate.bin");
        bool load_state(const std::string& filename = "savestate.bin");

        // Telemetry Disassembler: Decode current stellar instruction
        std::string disassemble_current_opcode() const;

        bool draw_flag; // Visual sensor array refresh beacon
        uint8_t display[64 * 32]; // Quantum monochrome display matrix
        uint8_t key[16]; // Lunar navigation keypad array

        uint8_t get_sound_timer() const { return sound_timer; }
        uint8_t get_delay_timer() const { return delay_timer; }
        uint16_t get_pc() const { return pc; }
        uint16_t get_index() const { return index; }
        uint8_t get_sp() const { return sp; }
        uint8_t get_register(uint8_t reg) const { return (reg < 16) ? v[reg] : 0; }
        uint16_t get_current_opcode() const { return opcode; }

    private:
        uint8_t memory[4096]; // Main spacecraft 4KB memory bank
        uint8_t v[16];        // 16 general-purpose propulsion registers (V0 to VF)
        uint16_t index;       // Stellar navigation index register
        uint16_t pc;          // Flight path program counter (starts at 0x200)
        uint16_t stack[16];   // Navigation waypoint subroutine stack
        uint8_t sp;           // Subroutine stack pointer
        uint8_t delay_timer;  // Spacecraft 60Hz delay chronometer
        uint8_t sound_timer;  // Spacecraft acoustic alert beacon
        uint16_t opcode;      // Current orbital execution instruction

        void initialise();    // Reset flight instruments to launch state
        void load_fonts();    // Load astronomical font glyphs into low memory
};

#endif
