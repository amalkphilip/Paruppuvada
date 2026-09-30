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
const int WIDTH = 64 * SCALE;
const int HEIGHT = 32 * SCALE;

// Default standard hex keypad mapping
const SDL_Keycode default_keymap[16] = {
    SDLK_x, SDLK_1, SDLK_2, SDLK_3,
    SDLK_q, SDLK_w, SDLK_e, SDLK_a,
    SDLK_s, SDLK_d, SDLK_z, SDLK_c,
    SDLK_4, SDLK_r, SDLK_f, SDLK_v
};

void audio_callback(void* userdata, uint8_t* stream, int len){
    static uint32_t sample_index = 0;
    int16_t* audio_buffer = (int16_t*) stream;
    int samples = len / 2;
    bool* beeping = (bool*) userdata;
    for(int i=0; i<samples; i++){
        if(*beeping){
            int16_t value = ((sample_index++ / 50) % 2) ? 3000 : -3000;
            audio_buffer[i] = value;
        } else {
            audio_buffer[i] = 0;
            sample_index = 0;
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
        case '+': bitmap = 002720; break;
        case '-': bitmap = 000700; break;
        case '/': bitmap = 011244; break;
        case '(': bitmap = 032223; break;
        case ')': bitmap = 062226; break;
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

void draw_game_over_popup(SDL_Renderer* renderer, const std::string& rom_name) {
    // Retro monochrome black & white modal popup
    SDL_Rect box = { WIDTH/2 - 150, HEIGHT/2 - 55, 300, 110 };
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &box);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &box);
    box.x += 2; box.y += 2; box.w -= 4; box.h -= 4;
    SDL_RenderDrawRect(renderer, &box);

    draw_text_hr(renderer, "- GAME OVER -", WIDTH/2 - (13*12)/2, HEIGHT/2 - 42, 3);
    
    std::string reason = "GAME OVER";
    if (rom_name.find("Tetris") != std::string::npos || rom_name.find("tetris") != std::string::npos) {
        reason = "BLOCKS REACHED TOP";
    } else if (rom_name.find("Blinky") != std::string::npos || rom_name.find("blinky") != std::string::npos) {
        reason = "NO LIVES REMAINING";
    }
    draw_text_hr(renderer, reason, WIDTH/2 - ((int)reason.length()*8)/2, HEIGHT/2 - 8, 2);
    
    std::string prompt = "ENTER / R: REPLAY   ESC: MENU";
    draw_text_hr(renderer, prompt, WIDTH/2 - ((int)prompt.length()*8)/2, HEIGHT/2 + 24, 2);
}

void draw_graphics(SDL_Renderer* renderer, const uint8_t* display, int cycles_per_frame, const std::string& rom_name, bool game_over = false){
    // Background: pure black
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Active pixels: pure white
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for(int y=0; y<32; y++){
        for(int x=0; x<64; x++){
            if(display[x + (y*64)] == 1){
                SDL_Rect rect = {x*SCALE, y*SCALE, SCALE, SCALE};
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }

    // --- On-screen SPEED bar HUD in top-left (Pure Black & White) ---
    SDL_Rect bg = { 8, 8, 172, 24 };
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &bg);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &bg);

    draw_text_hr(renderer, "SPEED", 12, 15, 2);

    SDL_Rect bar_frame = { 60, 14, 72, 12 };
    SDL_RenderDrawRect(renderer, &bar_frame);

    float ratio = (float)cycles_per_frame / 40.0f;
    if (ratio > 1.0f) ratio = 1.0f;
    if (ratio < 0.04f) ratio = 0.04f;
    int fill_w = (int)(ratio * 68);
    SDL_Rect bar_fill = { 62, 16, fill_w, 8 };
    SDL_RenderFillRect(renderer, &bar_fill);

    std::string speed_str = std::to_string(cycles_per_frame);
    draw_text_hr(renderer, speed_str, 138, 15, 2);

    // --- On-screen RESTART hint in top-right (Pure Black & White) ---
    SDL_Rect r_badge = { WIDTH - 120, 8, 112, 24 };
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderFillRect(renderer, &r_badge);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &r_badge);
    draw_text_hr(renderer, "R: RESTART", WIDTH - 114, 15, 2);

    if (game_over) {
        draw_game_over_popup(renderer, rom_name);
    }

    SDL_RenderPresent(renderer);
}

void map_key(Chip8& chip8, SDL_Keycode sym, uint8_t state, const std::string& rom_name) {
    if (rom_name.find("Blinky") != std::string::npos || rom_name.find("blinky") != std::string::npos) {
        // UP -> Key 3
        if (sym == SDLK_UP || sym == SDLK_w) chip8.key[3] = state;
        // DOWN -> Key 6
        else if (sym == SDLK_DOWN || sym == SDLK_s) chip8.key[6] = state;
        // LEFT -> Key 7
        else if (sym == SDLK_LEFT || sym == SDLK_a) chip8.key[7] = state;
        // RIGHT -> Key 8
        else if (sym == SDLK_RIGHT || sym == SDLK_d) chip8.key[8] = state;
        // Start / Pause / Resume -> Key 1
        else if (sym == SDLK_1 || sym == SDLK_SPACE || sym == SDLK_RETURN) chip8.key[1] = state;
        // Game Over continue / Key F
        else if (sym == SDLK_v || sym == SDLK_f) chip8.key[15] = state;
    } else if (rom_name.find("Tetris") != std::string::npos || rom_name.find("tetris") != std::string::npos) {
        // Rotate -> Key 5
        if (sym == SDLK_w || sym == SDLK_UP) chip8.key[5] = state;
        // Left -> Key 4
        else if (sym == SDLK_a || sym == SDLK_LEFT) chip8.key[4] = state;
        // Right -> Key 6
        else if (sym == SDLK_d || sym == SDLK_RIGHT) chip8.key[6] = state;
        // Drop -> Key 7
        else if (sym == SDLK_s || sym == SDLK_DOWN || sym == SDLK_SPACE) chip8.key[7] = state;
    } else if (rom_name.find("Pong") != std::string::npos || rom_name.find("pong") != std::string::npos) {
        // Player 1: UP / DOWN
        if (sym == SDLK_UP || sym == SDLK_w) chip8.key[1] = state;
        else if (sym == SDLK_DOWN || sym == SDLK_s) chip8.key[4] = state;
        // Player 2: NUM8 / NUM2 or I / K
        else if (sym == SDLK_KP_8 || sym == SDLK_i) chip8.key[12] = state;
        else if (sym == SDLK_KP_2 || sym == SDLK_k) chip8.key[13] = state;
    } else {
        // Standard hex keypad mapping
        for(int i=0; i<16; i++){
            if(sym == default_keymap[i]) chip8.key[i] = state;
        }
    }
}

void handle_input(Chip8& chip8, bool& running, int& cycles_per_frame, SDL_Window* window, const std::string& rom_name, bool& restart_requested){
    SDL_Event event;
    while(SDL_PollEvent(&event)){
        if(event.type == SDL_QUIT) running = false;
        if(event.type == SDL_KEYDOWN){
            if(event.key.keysym.sym == SDLK_ESCAPE) running = false;

            // In-game Restart hotkey ('R')
            if(event.key.keysym.sym == SDLK_r){
                restart_requested = true;
                return;
            }

            // Speed adjustment controls: Increase speed (+ / = / numpad+ / ])
            if(event.key.keysym.sym == SDLK_EQUALS || event.key.keysym.sym == SDLK_PLUS ||
               event.key.keysym.sym == SDLK_KP_PLUS || event.key.keysym.sym == SDLK_RIGHTBRACKET){
                if(cycles_per_frame < 100){
                    cycles_per_frame += (cycles_per_frame < 10 ? 1 : 2);
                    std::string title = "Chip-8 Emulator - Speed: " + std::to_string(cycles_per_frame) + " cycles/frame (" + std::to_string(cycles_per_frame * 60) + " Hz)";
                    SDL_SetWindowTitle(window, title.c_str());
                    std::cout << "[Speed] " << cycles_per_frame << " cycles/frame (" << (cycles_per_frame * 60) << " Hz)" << std::endl;
                }
            }
            // Speed adjustment controls: Decrease speed (- / numpad- / [)
            else if(event.key.keysym.sym == SDLK_MINUS || event.key.keysym.sym == SDLK_KP_MINUS ||
                    event.key.keysym.sym == SDLK_LEFTBRACKET){
                if(cycles_per_frame > 1){
                    cycles_per_frame -= (cycles_per_frame <= 10 ? 1 : 2);
                    if(cycles_per_frame < 1) cycles_per_frame = 1;
                    std::string title = "Chip-8 Emulator - Speed: " + std::to_string(cycles_per_frame) + " cycles/frame (" + std::to_string(cycles_per_frame * 60) + " Hz)";
                    SDL_SetWindowTitle(window, title.c_str());
                    std::cout << "[Speed] " << cycles_per_frame << " cycles/frame (" << (cycles_per_frame * 60) << " Hz)" << std::endl;
                }
            }

            map_key(chip8, event.key.keysym.sym, 1, rom_name);
        }
        if(event.type == SDL_KEYUP){
            map_key(chip8, event.key.keysym.sym, 0, rom_name);
        }
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
                    if(selection < 0) selection = (int)games.size() - 1;
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
        
        // Pure Black & White Menu
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_Rect border = { 20, 20, WIDTH - 40, HEIGHT - 40 };
        SDL_RenderDrawRect(renderer, &border);
        border.x += 2; border.y += 2; border.w -= 4; border.h -= 4;
        SDL_RenderDrawRect(renderer, &border);

        draw_text_hr(renderer, "SELECT A GAME", WIDTH/2 - (13*16)/2, 45, 4);
        
        for(size_t i=0; i<games.size(); i++){
            if((int)i == selection) {
                draw_text_hr(renderer, ">", 200, 120 + i*40, 3);
            }
            draw_text_hr(renderer, games[i], 240, 120 + i*40, 3);
        }
        
        draw_text_hr(renderer, "UP / DOWN: SELECT   ENTER: PLAY   ESC: QUIT", WIDTH/2 - (41*8)/2, 275, 2);

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
                if(event.key.keysym.sym == SDLK_ESCAPE) {
                    return false;
                }
                if(event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_SPACE) {
                    return true;
                }
            }
        }
        
        // Pure Black background
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        // Crisp White double border
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_Rect border = { 30, 20, WIDTH - 60, HEIGHT - 40 };
        SDL_RenderDrawRect(renderer, &border);
        border.x += 2; border.y += 2; border.w -= 4; border.h -= 4;
        SDL_RenderDrawRect(renderer, &border);
        
        draw_text_hr(renderer, "ESC: BACK", WIDTH - 150, 30, 2);
        
        if (rom_name.find("Tetris") != std::string::npos) {
            draw_text_hr(renderer, "- TETRIS CONTROLS -", WIDTH/2 - (19*12)/2, 45, 3);
            draw_text_hr(renderer, "W / UP ARROW     ROTATE", 140, 95, 2);
            draw_text_hr(renderer, "A / LEFT ARROW   MOVE LEFT", 140, 125, 2);
            draw_text_hr(renderer, "D / RIGHT ARROW  MOVE RIGHT", 140, 155, 2);
            draw_text_hr(renderer, "S / DOWN / SPACE DROP", 140, 185, 2);
        } else if (rom_name.find("Pong") != std::string::npos) {
            draw_text_hr(renderer, "- PONG CONTROLS -", WIDTH/2 - (17*12)/2, 45, 3);
            draw_text_hr(renderer, "PLAYER 1: UP / DOWN  or  W / S", 110, 105, 2);
            draw_text_hr(renderer, "PLAYER 2: NUM8 / NUM2  or  I / K", 110, 145, 2);
        } else if (rom_name.find("Blinky") != std::string::npos) {
            draw_text_hr(renderer, "- BLINKY (PAC-MAN) CONTROLS -", WIDTH/2 - (29*12)/2, 45, 3);
            draw_text_hr(renderer, "ARROWS  or  WASD  : MOVE PAC-MAN", 120, 100, 2);
            draw_text_hr(renderer, "SPACE   or  1     : START / PAUSE", 120, 135, 2);
            draw_text_hr(renderer, "MAZE GENERATES AT START (~8 SEC)", 120, 170, 2);
        }
        
        // Universal shortcuts info
        draw_text_hr(renderer, "R: RESTART GAME AT ANY TIME", WIDTH/2 - (27*8)/2, 215, 2);
        draw_text_hr(renderer, "SPEED: [+] FASTER   [-] SLOWER", WIDTH/2 - (30*8)/2, 240, 2);
        draw_text_hr(renderer, "PRESS ENTER OR SPACE TO START", WIDTH/2 - (29*8)/2, 270, 2);

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
            if(argc >= 2) app_running = false;
            continue; // Return to menu
        }

        Chip8 chip8;
        chip8.load_rom(rom_to_load);
        
        // Optimal default speed per game:
        // Blinky (CHIP-48) needs ~25 cycles/frame for smooth, responsive arcade speed
        int cycles_per_frame = 10;
        if (rom_to_load.find("Blinky") != std::string::npos || rom_to_load.find("blinky") != std::string::npos) {
            cycles_per_frame = 25;
        }

        std::string initial_title = "Chip-8 Emulator - Speed: " + std::to_string(cycles_per_frame) + " cycles/frame (" + std::to_string(cycles_per_frame * 60) + " Hz)";
        SDL_SetWindowTitle(window, initial_title.c_str());

        bool running = true;
        bool restart_requested = false;

        while(running){
            if(!chip8.is_game_over()){
                handle_input(chip8, running, cycles_per_frame, window, rom_to_load, restart_requested);
                if(restart_requested){
                    std::cout << "[System] Restarting game: " << rom_to_load << std::endl;
                    chip8.reset();
                    chip8.load_rom(rom_to_load);
                    restart_requested = false;
                    continue;
                }

                for(int i=0; i<cycles_per_frame; i++){
                    chip8.emulate_cycle();
                    if(chip8.is_game_over()) break;
                }
                chip8.update_timers();
                beeping = (chip8.get_sound_timer() > 0);

                draw_graphics(renderer, chip8.display, cycles_per_frame, rom_to_load, false);
                chip8.draw_flag = false;
            } else {
                beeping = false;
                draw_graphics(renderer, chip8.display, cycles_per_frame, rom_to_load, true);

                SDL_Event event;
                while(SDL_PollEvent(&event)){
                    if(event.type == SDL_QUIT){
                        running = false;
                        app_running = false;
                    }
                    if(event.type == SDL_KEYDOWN){
                        if(event.key.keysym.sym == SDLK_ESCAPE){
                            running = false; // Return to menu
                        }
                        else if(event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_SPACE || event.key.keysym.sym == SDLK_r){
                            std::cout << "[System] Replaying game: " << rom_to_load << std::endl;
                            chip8.reset();
                            chip8.load_rom(rom_to_load);
                        }
                    }
                }
            }
            SDL_Delay(16); // ~60 FPS
        }
        SDL_SetWindowTitle(window, "Chip-8 Emulator");
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
