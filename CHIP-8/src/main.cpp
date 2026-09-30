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

const int SCALE = 10; // Each pixel is 10x10 screen pixels
const int WIDTH = 64 * SCALE;
const int HEIGHT = 32 * SCALE;
const int CYCLES_PER_FRAME = 9; // ~540 Hz CPU clock (9 cycles * 60 FPS)

// Keypad hexadecimal index mapping to standard QWERTY keys
// Chip-8 Keypad:      QWERTY Keyboard:
// 1 2 3 C             1 2 3 4
// 4 5 6 D             Q W E R
// 7 8 9 E             A S D F
// A 0 B F             Z X C V
uint8_t keymap[16] = {
    SDLK_x, // 0 -> X
    SDLK_1, // 1 -> 1
    SDLK_2, // 2 -> 2
    SDLK_3, // 3 -> 3
    SDLK_q, // 4 -> Q
    SDLK_w, // 5 -> W
    SDLK_e, // 6 -> E
    SDLK_a, // 7 -> A
    SDLK_s, // 8 -> S
    SDLK_d, // 9 -> D
    SDLK_z, // A -> Z
    SDLK_c, // B -> C
    SDLK_4, // C -> 4
    SDLK_r, // D -> R
    SDLK_f, // E -> F
    SDLK_v  // F -> V
};

void audio_callback(void* userdata, uint8_t* stream, int len){
    static uint32_t sample_index = 0;
    int16_t* audio_buffer = (int16_t*) stream;
    int samples = len / 2;

    bool* beeping = (bool*) userdata;
    for(int i = 0; i < samples; i++){
        if(*beeping){
            // Generate 440 Hz square wave (sample rate 44100)
            // Period = 44100 / 440 ≈ 100 samples (50 high, 50 low)
            int16_t value = ((sample_index++ / 50) % 2) ? 3000 : -3000;
            audio_buffer[i] = value;
        } else {
            audio_buffer[i] = 0; // Silence
            sample_index = 0;
        }
    }
}

void draw_graphics(SDL_Renderer* renderer, Chip8& chip8){
    // Clear screen to black
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Draw active white pixels
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for(int y = 0; y < 32; y++){
        for(int x = 0; x < 64; x++){
            if(chip8.display[x + (y * 64)] == 1){
                SDL_Rect rect = {x * SCALE, y * SCALE, SCALE, SCALE};
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }
    SDL_RenderPresent(renderer);
}

void handle_input(Chip8& chip8, bool& running){
    SDL_Event event;

    while(SDL_PollEvent(&event)){
        if(event.type == SDL_QUIT) {
            running = false;
        } else if(event.type == SDL_KEYDOWN){
            if(event.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
            }
            // Check which Chip-8 key was pressed
            for(int i = 0; i < 16; i++){
                if(event.key.keysym.sym == keymap[i]){
                    chip8.key[i] = 1;
                }
            }
        } else if(event.type == SDL_KEYUP){
            for(int i = 0; i < 16; i++){
                if(event.key.keysym.sym == keymap[i]){
                    chip8.key[i] = 0;
                }
            }
        }
    }
}

int main(int argc, char** argv){
    if(argc < 2){
        std::cerr << "Usage: " << argv[0] << " <ROM file>" << std::endl;
        return 1;
    }

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0){
        std::cerr << "SDL Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Audio setup
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
    if(audio_device == 0){
        std::cerr << "Failed to open audio: " << SDL_GetError() << std::endl;
    } else {
        SDL_PauseAudioDevice(audio_device, 0);
    }

    SDL_Window* window = SDL_CreateWindow(
        "Chip-8 Emulator",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WIDTH,
        HEIGHT,
        SDL_WINDOW_SHOWN
    );

    if(!window){
        std::cerr << "Window error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if(!renderer){
        std::cerr << "Renderer error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    Chip8 chip8;
    chip8.load_rom(argv[1]);

    const uint32_t FRAME_DELAY_MS = 1000 / 60; // ~16.6 ms per frame (60 Hz)
    bool running = true;

    while(running){
        uint32_t frame_start = SDL_GetTicks();

        handle_input(chip8, running);

        // Run CPU cycles for this frame
        for(int i = 0; i < CYCLES_PER_FRAME; i++){
            chip8.emulate_cycle();
        }

        // Decrement timers at 60 Hz
        chip8.update_timers();
        beeping = (chip8.get_sound_timer() > 0);

        // Draw if screen state was changed
        if(chip8.draw_flag){
            draw_graphics(renderer, chip8);
            chip8.draw_flag = false;
        }

        // Cap frame rate to 60 FPS
        uint32_t frame_time = SDL_GetTicks() - frame_start;
        if(frame_time < FRAME_DELAY_MS){
            SDL_Delay(FRAME_DELAY_MS - frame_time);
        }
    }

    if(audio_device != 0) SDL_CloseAudioDevice(audio_device);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}