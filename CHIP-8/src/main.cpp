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

void draw_char_hr(SDL_Renderer* renderer, char c, int x, int y, int scale) {
    uint16_t bitmap = 0;
    switch(c) {
        case 'A': bitmap = 075755; break;
        case 'B': bitmap = 065656; break;
        case 'C': bitmap = 074447; break;
        case 'D': bitmap = 065556; break;
        case 'E': bitmap = 074747; break;
        case 'F': bitmap = 074744; break;
        case 'G': bitmap = 074757; break;
        case 'H': bitmap = 055755; break;
        case 'I': bitmap = 072227; break;
        case 'J': bitmap = 011157; break;
        case 'K': bitmap = 055655; break;
        case 'L': bitmap = 044447; break;
        case 'M': bitmap = 057555; break;
        case 'N': bitmap = 065555; break;
        case 'O': bitmap = 075557; break;
        case 'P': bitmap = 075744; break;
        case 'Q': bitmap = 075571; break;
        case 'R': bitmap = 075655; break;
        case 'S': bitmap = 074717; break;
        case 'T': bitmap = 072222; break;
        case 'U': bitmap = 055557; break;
        case 'V': bitmap = 055552; break;
        case 'W': bitmap = 055575; break;
        case 'X': bitmap = 055255; break;
        case 'Y': bitmap = 055222; break;
        case 'Z': bitmap = 071247; break;
        case '0': bitmap = 075557; break;
        case '1': bitmap = 026227; break;
        case '2': bitmap = 071747; break;
        case '3': bitmap = 071717; break;
        case '4': bitmap = 055711; break;
        case '5': bitmap = 074717; break;
        case '6': bitmap = 074757; break;
        case '7': bitmap = 071111; break;
        case '8': bitmap = 075757; break;
        case '9': bitmap = 075717; break;
        case '>': bitmap = 042124; break;
        case ':': bitmap = 002020; break;
        case '-': bitmap = 000700; break;
        case '[': bitmap = 064446; break;
        case ']': bitmap = 031113; break;
        case ' ': bitmap = 000000; break;
    }
    for (int row=0; row<5; row++) {
        int r_bits = (bitmap >> (12 - row*3)) & 7;
        for (int col=0; col<3; col++) {
            if (r_bits & (4 >> col)) {
                SDL_Rect rect = {x + col*scale, y + row*scale, scale, scale};
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }
}

void draw_text_hr(SDL_Renderer* renderer, const std::string& str, int x, int y, int scale = 2) {
    for(size_t i=0; i<str.length(); i++){
        draw_char_hr(renderer, str[i], x + i*(4*scale), y, scale);
    }
}

std::string run_menu(SDL_Renderer* renderer) {
    std::vector<std::string> games = {"TETRIS", "PONG", "BLINKY"};
    std::vector<std::string> files = {"roms/Tetris.ch8", "roms/Pong.ch8", "roms/Blinky.ch8"};
    int selection = 0;
    SDL_Event event;
    bool selecting = true;
    while(selecting){
        while(SDL_PollEvent(&event)){
            if(event.type == SDL_QUIT) return "QUIT";
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
                    return "QUIT";
                }
            }
        }
        
        SDL_SetRenderDrawColor(renderer, 10, 10, 30, 255); // Dark blue background
        SDL_RenderClear(renderer);
        
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White text
        draw_text_hr(renderer, "SELECT A GAME", WIDTH/2 - (13*16)/2, 40, 4);
        
        for(size_t i=0; i<games.size(); i++){
            if((int)i == selection) {
                draw_text_hr(renderer, ">", 150, 120 + i*40, 3);
            }
            draw_text_hr(renderer, games[i], 200, 120 + i*40, 3);
        }
        
        draw_text_hr(renderer, "PRESS ENTER TO START", WIDTH/2 - (20*8)/2, 280, 2);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    return "QUIT";
}

bool show_popup(SDL_Renderer* renderer, const std::string& rom_name) {
    SDL_Event event;
    bool in_popup = true;
    while(in_popup){
        while(SDL_PollEvent(&event)){
            if(event.type == SDL_QUIT) return false;
            if(event.type == SDL_KEYDOWN){
                if(event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_SPACE) {
                    return true;
                }
            }
        }
        
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 255); // Dark grey background
        SDL_RenderClear(renderer);
        
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        
        // Draw popup border box
        SDL_Rect border = { 50, 30, WIDTH - 100, HEIGHT - 60 };
        SDL_RenderDrawRect(renderer, &border);
        border.x += 2; border.y += 2; border.w -= 4; border.h -= 4;
        SDL_RenderDrawRect(renderer, &border);
        
        draw_text_hr(renderer, "[X] ESC TO CLOSE", WIDTH - 200, 40, 2);
        
        if (rom_name == "roms/Tetris.ch8") {
            draw_text_hr(renderer, "- TETRIS CONTROLS -", WIDTH/2 - (19*12)/2, 70, 3);
            draw_text_hr(renderer, "W   ROTATE", 150, 140, 3);
            draw_text_hr(renderer, "A   LEFT", 150, 180, 3);
            draw_text_hr(renderer, "D   RIGHT", 350, 140, 3);
            draw_text_hr(renderer, "S   DROP", 350, 180, 3);
        } else if (rom_name == "roms/Pong.ch8") {
            draw_text_hr(renderer, "- PONG CONTROLS -", WIDTH/2 - (17*12)/2, 70, 3);
            draw_text_hr(renderer, "PLAYER 1:", 100, 130, 3);
            draw_text_hr(renderer, "UP   DOWN", 100, 170, 2);
            draw_text_hr(renderer, "ARROW KEYS", 100, 200, 2);
            draw_text_hr(renderer, "PLAYER 2:", 350, 130, 3);
            draw_text_hr(renderer, "NUM8 NUM2", 350, 170, 2);
        } else if (rom_name == "roms/Blinky.ch8") {
            draw_text_hr(renderer, "- BLINKY CONTROLS -", WIDTH/2 - (19*12)/2, 70, 3);
            draw_text_hr(renderer, "USE THE", 200, 140, 4);
            draw_text_hr(renderer, "ARROW KEYS", 200, 200, 4);
        }
        
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    return false;
}

int main(int argc, char** argv){
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

    bool app_running = true;
    while(app_running) {
        std::string rom_to_load = "";
        if(argc >= 2) rom_to_load = argv[1];
        
        if(rom_to_load == "") {
            rom_to_load = run_menu(renderer);
        }

        if(rom_to_load == "QUIT" || rom_to_load == "") {
            app_running = false;
            break;
        }

        // Show the popup controls screen
        if (!show_popup(renderer, rom_to_load)) {
            app_running = false;
            break; // User completely exited the app from popup
        }

        // Set dynamic keymap based on ROM
        keymap[0] = SDLK_x; keymap[1] = SDLK_1; keymap[2] = SDLK_2; keymap[3] = SDLK_3;
        keymap[4] = SDLK_q; keymap[5] = SDLK_w; keymap[6] = SDLK_e; keymap[7] = SDLK_a;
        keymap[8] = SDLK_s; keymap[9] = SDLK_d; keymap[10] = SDLK_z; keymap[11] = SDLK_c;
        keymap[12] = SDLK_4; keymap[13] = SDLK_r; keymap[14] = SDLK_f; keymap[15] = SDLK_v;

        if (rom_to_load == "roms/Tetris.ch8") {
            keymap[5] = SDLK_w; // Rotate
            keymap[4] = SDLK_a; // Left
            keymap[6] = SDLK_d; // Right
            keymap[7] = SDLK_s; // Drop
        } else if (rom_to_load == "roms/Pong.ch8") {
            keymap[1] = SDLK_UP;     // P1 Up
            keymap[4] = SDLK_DOWN;   // P1 Down
            keymap[0xC] = SDLK_KP_8; // P2 Up (CHIP-8 Key C)
            keymap[0xD] = SDLK_KP_2; // P2 Down (CHIP-8 Key D)
        } else if (rom_to_load == "roms/Blinky.ch8") {
            keymap[3] = SDLK_UP;
            keymap[6] = SDLK_DOWN;
            keymap[7] = SDLK_LEFT;
            keymap[8] = SDLK_RIGHT;
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
        // If a ROM was provided via CLI, quit after it finishes
        if(argc >= 2) {
            app_running = false;
        }
    }

    if(audio_device != 0) SDL_CloseAudioDevice(audio_device);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}