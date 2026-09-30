#ifndef CHIP8_H
#define CHIP8_H

#include <cstdint>
#include <string>

class Chip8{
    public:
        Chip8();
        void load_rom(const std::string& filename); // To load a game file
        void emulate_cycle(); // To execute one instruction
        void update_timers(); // Update delay and sound timers (call once per frame at 60Hz)
        bool draw_flag; // When we need to redraw the screen
        uint8_t display[64*32];
        uint8_t key[16]; // Keyboard of 16 keys
        uint8_t get_sound_timer() const { return sound_timer; } // Sound timer value
        void reset();
        bool is_game_over() const { return game_over; }
        uint16_t get_pc() const { return pc; }
        uint8_t get_v(int i) const { return (i >= 0 && i < 16) ? v[i] : 0; }
        const std::string& get_current_rom() const { return current_rom; }
    private:
        uint8_t memory[4096]; // Memory of 4KB
        uint8_t v[16]; // 16 registers, V0 to VF
        uint16_t index; // Index register for memory addresses
        uint16_t pc; // Program counter
        uint16_t stack[16];
        uint8_t sp; // Stack pointer
        uint8_t delay_timer; // Counts down at 60Hz
        uint8_t sound_timer; // Beeps when greater than 0, counts down at 60Hz
        uint16_t opcode; // Current instruction
        bool game_over;
        std::string current_rom;
        void initialise(); // Initialises everything
        void load_fonts(); // Loads font (0-9, A-F)
};

#endif