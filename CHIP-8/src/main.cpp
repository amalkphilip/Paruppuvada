#include "chip8.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

const int SCALE = 10; // Each pixel is 10x10 screen pixels
const int WIDTH = 64*SCALE;
const int HEIGHT = 32*SCALE;

// Keyboard mapping
SDL_Keycode keymap[16] = {
    SDLK_x, SDLK_1, SDLK_2, SDLK_3,
    SDLK_q, SDLK_w, SDLK_e, SDLK_a,
    SDLK_s, SDLK_d, SDLK_z, SDLK_c,
    SDLK_4, SDLK_r, SDLK_f, SDLK_v
};

void audio_callback(void* userdata, uint8_t* stream, int len){
    static uint32_t sample_index = 0;
    int16_t* audio_buffer = (int16_t*) stream;
    int samples = len/2;
    bool* beeping = (bool*) userdata;
    for(int i=0; i<samples; i++){
        if(*beeping){
            int16_t value = ((sample_index++ / 50) % 2) ? 3000 : -3000;
            audio_buffer[i] = value;
        }else{
            audio_buffer[i] = 0;
            sample_index = 0;
        }
    }
}

void draw_graphics(SDL_Renderer* renderer, const uint8_t* display){
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for(int y=0; y<32; y++){
        for(int x=0; x<64; x++){
            if(display[x + (y*64)] == 1){
                SDL_Rect rect = {x*SCALE, y*SCALE, SCALE, SCALE};
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }
    SDL_RenderPresent(renderer);
}

void handle_input(Chip8& chip8, bool& running){
    SDL_Event event;
    while(SDL_PollEvent(&event)){
        if(event.type == SDL_QUIT) running = false;
        if(event.type == SDL_KEYDOWN){
            if(event.key.keysym.sym == SDLK_ESCAPE) running = false;
            for(int i=0; i<16; i++){
                if(event.key.keysym.sym == keymap[i]) chip8.key[i] = 1;
            }
        }
        if(event.type == SDL_KEYUP){
            for(int i=0; i<16; i++){
                if(event.key.keysym.sym == keymap[i]) chip8.key[i] = 0;
            }
        }
    }
}

void draw_char(uint8_t* display, char c, int x, int y) {
    uint16_t bitmap = 0;
    switch(c) {
        case 'T': bitmap = 072222; break;
        case 'E': bitmap = 074747; break;
        case 'R': bitmap = 075655; break;
        case 'I': bitmap = 072227; break;
        case 'S': bitmap = 074717; break;
        case 'P': bitmap = 075744; break;
        case 'O': bitmap = 075557; break;
        case 'N': bitmap = 057555; break;
        case 'G': bitmap = 074757; break;
        case 'B': bitmap = 065656; break;
        case 'L': bitmap = 044447; break;
        case 'K': bitmap = 056465; break;
        case 'Y': bitmap = 055222; break;
        case '>': bitmap = 042124; break;
        case ' ': bitmap = 000000; break;
    }
    for (int row=0; row<5; row++) {
        int r_bits = (bitmap >> (12 - row*3)) & 7;
        for (int col=0; col<3; col++) {
            if (r_bits & (4 >> col)) {
                if (x+col < 64 && y+row < 32)
                    display[(x+col) + (y+row)*64] = 1;
            }
        }
    }
}

void draw_text(uint8_t* display, const std::string& str, int x, int y) {
    for(size_t i=0; i<str.length(); i++){
        draw_char(display, str[i], x + i*4, y);
    }
}

std::string run_menu(SDL_Renderer* renderer) {
    std::vector<std::string> games = {"TETRIS", "PONG", "BLINKY"};
    std::vector<std::string> files = {"roms/Tetris.ch8", "roms/Pong.ch8", "roms/Blinky.ch8"};
    int selection = 0;
    uint8_t menu_display[64*32];
    SDL_Event event;
    bool selecting = true;
    while(selecting){
        while(SDL_PollEvent(&event)){
            if(event.type == SDL_QUIT) return "";
            if(event.type == SDL_KEYDOWN){
                if(event.key.keysym.sym == SDLK_UP) {
                    selection--;
                    if(selection < 0) selection = games.size() - 1;
                }
                else if(event.key.keysym.sym == SDLK_DOWN) {
                    selection++;
                    if(selection >= (int)games.size()) selection = 0;
                }
                else if(event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_SPACE) {
                    return files[selection];
                }
                else if(event.key.keysym.sym == SDLK_ESCAPE) {
                    return "";
                }
            }
        }
        memset(menu_display, 0, sizeof(menu_display));
        for(size_t i=0; i<games.size(); i++){
            if((int)i == selection) {
                draw_text(menu_display, ">", 10, 8 + i*8);
            }
            draw_text(menu_display, games[i], 16, 8 + i*8);
        }
        draw_graphics(renderer, menu_display);
        SDL_Delay(16);
    }
    return "";
}

int main(int argc, char** argv){
    std::string rom_to_load = "";
    if(argc >= 2) rom_to_load = argv[1];

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0){
        std::cerr << "SDL Error: " << SDL_GetError() << std::endl;
        return 1;
    }
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
    if(audio_device == 0) std::cerr << "Failed to open audio: " << SDL_GetError() << std::endl;
    else SDL_PauseAudioDevice(audio_device, 0);

    SDL_Window* window = SDL_CreateWindow("Chip-8 Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if(rom_to_load == "") rom_to_load = run_menu(renderer);

    if(rom_to_load == "") {
        if(audio_device != 0) SDL_CloseAudioDevice(audio_device);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    }

    Chip8 chip8;
    chip8.load_rom(rom_to_load);
    
    bool running = true;
    while(running){
        handle_input(chip8, running);
        for(int i=0; i<10; i++){
            chip8.emulate_cycle();
        }
        chip8.update_timers();
        beeping = (chip8.get_sound_timer() > 0);

        draw_graphics(renderer, chip8.display);
        chip8.draw_flag = false;
        SDL_Delay(16); // ~60 FPS
    }
    if(audio_device != 0) SDL_CloseAudioDevice(audio_device);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}