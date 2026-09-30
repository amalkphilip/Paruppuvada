#include "chip8.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <cstdint>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>

// Mission Control Status: Stellar
// Cosmo-Polo Space Exploration Visual & Audio Telemetry Subsystem

const int SCALE = 10; // Quantum sensor magnification factor (640x320 viewport)
const int WIDTH = 64 * SCALE;
const int HEIGHT = 32 * SCALE;

// Configurable Celestial Color Schemes
const std::vector<ColorPalette> PALETTES = {
    {"CRT Matrix Green", 5, 20, 5, 57, 255, 20},     // Classic Green Phosphor
    {"Amber Phosphor", 22, 12, 0, 255, 176, 0},      // Retro Terminal Amber
    {"Cyberpunk Neon", 11, 5, 24, 255, 0, 127},      // Deep Space Magenta / Neon
    {"Deep Space White", 0, 0, 0, 240, 240, 255},    // Clean Monochrome
    {"Solar Flare Gold", 15, 10, 0, 255, 215, 0}     // Solar Radiation Gold
};

// Spacecraft Keypad Mapping (Hexadecimal Array -> QWERTY Keyboard)
// 1 2 3 C   -->   1 2 3 4
// 4 5 6 D   -->   Q W E R
// 7 8 9 E   -->   A S D F
// A 0 B F   -->   Z X C V
const uint8_t keymap[16] = {
    SDLK_x, // 0x0 -> X
    SDLK_1, // 0x1 -> 1
    SDLK_2, // 0x2 -> 2
    SDLK_3, // 0x3 -> 3
    SDLK_q, // 0x4 -> Q
    SDLK_w, // 0x5 -> W
    SDLK_e, // 0x6 -> E
    SDLK_a, // 0x7 -> A
    SDLK_s, // 0x8 -> S
    SDLK_d, // 0x9 -> D
    SDLK_z, // 0xA -> Z
    SDLK_c, // 0xB -> C
    SDLK_4, // 0xC -> 4
    SDLK_r, // 0xD -> R
    SDLK_f, // 0xE -> F
    SDLK_v  // 0xF -> V
};

void audio_callback(void* userdata, uint8_t* stream, int len) {
    static uint32_t sample_index = 0;
    int16_t* audio_buffer = reinterpret_cast<int16_t*>(stream);
    int samples = len / 2;

    bool* beeping = reinterpret_cast<bool*>(userdata);
    for (int i = 0; i < samples; i++) {
        if (*beeping) {
            // Spacecraft 440 Hz Acoustic Alert Signal (Square Wave)
            int16_t value = ((sample_index++ / 50) % 2) ? 3000 : -3000;
            audio_buffer[i] = value;
        } else {
            audio_buffer[i] = 0; // Space vacuum silence
            sample_index = 0;
        }
    }
}

void draw_graphics(SDL_Renderer* renderer, const Chip8& chip8, size_t palette_idx) {
    const ColorPalette& pal = PALETTES[palette_idx % PALETTES.size()];

    // Render Deep Space Background
    SDL_SetRenderDrawColor(renderer, pal.bg_r, pal.bg_g, pal.bg_b, 255);
    SDL_RenderClear(renderer);

    // Render Active Quantum Pixels
    SDL_SetRenderDrawColor(renderer, pal.fg_r, pal.fg_g, pal.fg_b, 255);
    for (int y = 0; y < 32; y++) {
        for (int x = 0; x < 64; x++) {
            if (chip8.display[x + (y * 64)] == 1) {
                SDL_Rect rect = {x * SCALE, y * SCALE, SCALE, SCALE};
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }
    SDL_RenderPresent(renderer);
}

void update_window_title(SDL_Window* window, int cycles_per_frame, size_t palette_idx, bool paused) {
    std::stringstream ss;
    ss << "CHIP-8 Spacecraft Core [Status: " << (paused ? "PAUSED" : "STELLAR") << "] | "
       << "Warp Speed: " << cycles_per_frame << " c/f (" << (cycles_per_frame * 60) << " Hz) | "
       << "Palette: " << PALETTES[palette_idx % PALETTES.size()].name << " | "
       << "[F5:Save | F6:Load | +/-:Speed | TAB:Color | P:Pause | T:Telemetry]";
    SDL_SetWindowTitle(window, ss.str().c_str());
}

void handle_input(Chip8& chip8, bool& running, int& cycles_per_frame, size_t& palette_idx, bool& paused, SDL_Window* window) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            running = false;
        } else if (event.type == SDL_KEYDOWN) {
            SDL_Keycode key = event.key.keysym.sym;

            if (key == SDLK_ESCAPE) {
                running = false;
            }
            // Feature: Savestate Management
            else if (key == SDLK_F5) {
                chip8.save_state("savestate.bin");
            } else if (key == SDLK_F6 || key == SDLK_F8) {
                chip8.load_state("savestate.bin");
            }
            // Feature: Configurable Emulation Speed (+ / -)
            else if (key == SDLK_EQUALS || key == SDLK_PLUS || key == SDLK_KP_PLUS || key == SDLK_RIGHTBRACKET) {
                if (cycles_per_frame < 60) cycles_per_frame += 2;
                update_window_title(window, cycles_per_frame, palette_idx, paused);
                std::cout << "[PROPULSION] Warp Speed Increased: " << cycles_per_frame << " cycles/frame (" << (cycles_per_frame * 60) << " Hz)" << std::endl;
            } else if (key == SDLK_MINUS || key == SDLK_UNDERSCORE || key == SDLK_KP_MINUS || key == SDLK_LEFTBRACKET) {
                if (cycles_per_frame > 1) cycles_per_frame -= 2;
                update_window_title(window, cycles_per_frame, palette_idx, paused);
                std::cout << "[PROPULSION] Warp Speed Decreased: " << cycles_per_frame << " cycles/frame (" << (cycles_per_frame * 60) << " Hz)" << std::endl;
            }
            // Feature: Custom Display Color Schemes (TAB or F1)
            else if (key == SDLK_TAB || key == SDLK_F1) {
                palette_idx = (palette_idx + 1) % PALETTES.size();
                update_window_title(window, cycles_per_frame, palette_idx, paused);
                chip8.draw_flag = true;
                std::cout << "[OPTICS] Celestial Palette Switched: " << PALETTES[palette_idx].name << std::endl;
            }
            // Feature: Pause & Single-Step Trajectory
            else if (key == SDLK_p || key == SDLK_SPACE) {
                paused = !paused;
                update_window_title(window, cycles_per_frame, palette_idx, paused);
                std::cout << "[MISSION CONTROL] Orbital Execution " << (paused ? "PAUSED" : "RESUMED") << std::endl;
            } else if (key == SDLK_o || key == SDLK_n) {
                if (paused) {
                    std::cout << "[STEP] " << chip8.disassemble_current_opcode() << std::endl;
                    chip8.emulate_cycle();
                    chip8.draw_flag = true;
                }
            }
            // Cosmo-Polo Telemetry Query Trigger
            else if (key == SDLK_t) {
                cosmo_polo_telemetry();
            }

            // Keypad Telemetry Matrix Input
            for (int i = 0; i < 16; i++) {
                if (key == keymap[i]) {
                    chip8.key[i] = 1;
                }
            }
        } else if (event.type == SDL_KEYUP) {
            SDL_Keycode key = event.key.keysym.sym;
            for (int i = 0; i < 16; i++) {
                if (key == keymap[i]) {
                    chip8.key[i] = 0;
                }
            }
        }
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <ROM file>" << std::endl;
        std::cerr << "Mission Control Status: Stellar - System Ready." << std::endl;
        return 1;
    }

    // Initialize Cosmo-Polo Flight Telemetry
    cosmo_polo_telemetry();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cerr << "[MISSION CONTROL] SDL Hardware Initialization Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Audio Subsystem Setup
    bool beeping = false;
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = 44100;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 2048;
    want.callback = audio_callback;
    want.userdata = &beeping;

    SDL_AudioDeviceID audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (audio_device == 0) {
        std::cerr << "[MISSION CONTROL] Acoustic Beacon Warning: " << SDL_GetError() << std::endl;
    } else {
        SDL_PauseAudioDevice(audio_device, 0);
    }

    // Viewport & Optical Matrix Setup
    SDL_Window* window = SDL_CreateWindow(
        "CHIP-8 Spacecraft Core [Status: Stellar]",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WIDTH,
        HEIGHT,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        std::cerr << "[MISSION CONTROL] Viewport Creation Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "[MISSION CONTROL] Renderer Initialization Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Initialize Spacecraft Virtual Machine
    Chip8 chip8;
    chip8.load_rom(argv[1]);

    int cycles_per_frame = 9; // Default ~540 Hz propulsion cadence
    size_t palette_idx = 0;   // Default CRT Matrix Green
    bool paused = false;
    bool running = true;
    const uint32_t FRAME_DELAY_MS = 1000 / 60; // 60 Hz Spacecraft Chronometer Target (~16.6ms)

    update_window_title(window, cycles_per_frame, palette_idx, paused);

    std::cout << "[MISSION CONTROL] All systems nominal. Launching orbital loop..." << std::endl;

    // Main Orbital Execution Loop
    while (running) {
        uint32_t frame_start = SDL_GetTicks();

        handle_input(chip8, running, cycles_per_frame, palette_idx, paused, window);

        if (!paused) {
            // Execute CPU orbital clock cycles for current frame
            for (int i = 0; i < cycles_per_frame; i++) {
                chip8.emulate_cycle();
            }

            // Decrement flight chronometers at 60 Hz
            chip8.update_timers();
            beeping = (chip8.get_sound_timer() > 0);
        } else {
            beeping = false;
        }

        // Render optical sensor array if updated
        if (chip8.draw_flag) {
            draw_graphics(renderer, chip8, palette_idx);
            chip8.draw_flag = false;
        }

        // Frame rate capping to 60 FPS
        uint32_t frame_time = SDL_GetTicks() - frame_start;
        if (frame_time < FRAME_DELAY_MS) {
            SDL_Delay(FRAME_DELAY_MS - frame_time);
        }
    }

    // Clean Spacecraft Shutdown
    if (audio_device != 0) SDL_CloseAudioDevice(audio_device);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    std::cout << "[MISSION CONTROL] Spacecraft safely docked. Mission Control Status: Stellar." << std::endl;
    return 0;
}
