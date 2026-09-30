#include "chip8.h"
#include "retro_computer_ui.h"
#include <SDL2/SDL.h>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

using retro_gui::RetroComputerUI;
using retro_gui::VirtualKey;
using retro_gui::ComputerTheme;
using retro_gui::PaletteID;
using retro_gui::Waveform;

const int CRT_WIDTH = 512;
const int CRT_HEIGHT = 256;
const int WINDOW_W = 1280;
const int WINDOW_H = 800;

SDL_Keycode keymap[16] = {
    SDLK_x, SDLK_1, SDLK_2, SDLK_3,
    SDLK_q, SDLK_w, SDLK_e, SDLK_a,
    SDLK_s, SDLK_d, SDLK_z, SDLK_c,
    SDLK_4, SDLK_r, SDLK_f, SDLK_v
};

Waveform current_waveform = Waveform::SINE;

void audio_callback(void* userdata, uint8_t* stream, int len){
    static double phase = 0.0;
    int16_t* audio_buffer = (int16_t*) stream;
    int samples = len / 2;
    bool* beeping = (bool*) userdata;
    
    double freq = 440.0; 
    double phase_inc = freq / 44100.0;

    for(int i=0; i<samples; i++){
        if(*beeping){
            float sample = retro_gui::waveformSample(current_waveform, phase);
            audio_buffer[i] = (int16_t)(sample * 3000.0f);
            phase += phase_inc;
            if(phase >= 1.0) phase -= 1.0;
        } else {
            audio_buffer[i] = 0;
            phase = 0.0;
        }
    }
}



uint8_t fg_r = 0x8F, fg_g = 0xEA, fg_b = 0x9B;
uint8_t bg_r = 0x0B, bg_g = 0x1F, bg_b = 0x10;

void draw_graphics(SDL_Renderer* renderer, const uint8_t* display){
    float scale_x = (float)CRT_WIDTH / 64.0f;
    float scale_y = (float)CRT_HEIGHT / 32.0f;

    SDL_SetRenderDrawColor(renderer, bg_r, bg_g, bg_b, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    for(int y=0; y<32; y++){
        for(int x=0; x<64; x++){
            if(display[x + (y*64)] == 1){
                int px = (int)(x*scale_x);
                int py = (int)(y*scale_y);
                int pw = (int)scale_x;
                int ph = (int)scale_y;
                
                // Pure crisp CHIP-8 pixel with 1px grid gap
                SDL_SetRenderDrawColor(renderer, fg_r, fg_g, fg_b, 255);
                SDL_Rect rect = { px, py, pw - 1, ph - 1 };
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }
}

int cycles_per_frame = 12;
std::string current_rom_path = "";
Chip8 chip8;
bool app_running = true;

void enforce_60fps(Uint64 frame_start) {
    Uint64 frame_end = SDL_GetPerformanceCounter();
    double elapsed = (double)(frame_end - frame_start);
    double frame_duration = (double)SDL_GetPerformanceFrequency() / 60.0;
    if (elapsed < frame_duration) {
        double delay_ms = (frame_duration - elapsed) * 1000.0 / (double)SDL_GetPerformanceFrequency();
        SDL_Delay((Uint32)delay_ms);
    }
}

void handle_global_ui_keyboard(const SDL_Event& event, RetroComputerUI& ui) {
    if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
        bool pressed = (event.type == SDL_KEYDOWN);
        for (int i = 0; i < 16; i++) {
            if (event.key.keysym.sym == keymap[i]) {
                ui.setVirtualKeyPressed(i, pressed);
            }
        }
        if (event.key.keysym.sym == SDLK_UP) ui.setVirtualKeyPressed(retro_gui::VirtualKey::ARROW_UP, pressed);
        if (event.key.keysym.sym == SDLK_DOWN) ui.setVirtualKeyPressed(retro_gui::VirtualKey::ARROW_DOWN, pressed);
        if (event.key.keysym.sym == SDLK_LEFT) ui.setVirtualKeyPressed(retro_gui::VirtualKey::ARROW_LEFT, pressed);
        if (event.key.keysym.sym == SDLK_RIGHT) ui.setVirtualKeyPressed(retro_gui::VirtualKey::ARROW_RIGHT, pressed);
        if (event.key.keysym.sym == SDLK_SPACE) ui.setVirtualKeyPressed(retro_gui::VirtualKey::SPACE, pressed);
        if (event.key.keysym.sym == SDLK_s) ui.setVirtualKeyPressed(retro_gui::VirtualKey::SAVE, pressed);
        if (event.key.keysym.sym == SDLK_l) ui.setVirtualKeyPressed(retro_gui::VirtualKey::LOAD, pressed);
        
        if (pressed) {
            if (event.key.keysym.sym == SDLK_PLUS || event.key.keysym.sym == SDLK_KP_PLUS || event.key.keysym.sym == SDLK_EQUALS) {
                ui.setSpeedIndex(ui.getSpeedIndex() + 1);
            }
            if (event.key.keysym.sym == SDLK_MINUS || event.key.keysym.sym == SDLK_KP_MINUS) {
                ui.setSpeedIndex(ui.getSpeedIndex() - 1);
            }
        }
    }
}

std::string show_load_menu(SDL_Renderer* renderer, RetroComputerUI& ui, SDL_Texture* crt_tex) {
    std::vector<std::string> games;
    std::vector<std::string> files;
    
    if (fs::exists("roms")) {
        for (const auto& entry : fs::directory_iterator("roms")) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".sav") {
                    std::string name = entry.path().stem().string();
                    std::transform(name.begin(), name.end(), name.begin(), ::toupper);
                    games.push_back(name);
                    files.push_back(entry.path().generic_string());
                }
            }
        }
    }
    if (games.empty()) {
        games.push_back("NO SAVES FOUND");
        files.push_back("");
    }

    int selection = 0;
    int scroll_offset = 0;
    const int max_visible = 4;
    
    SDL_Event event;
    while(true){
        Uint64 frame_start = SDL_GetPerformanceCounter();
        while(SDL_PollEvent(&event)){
            if(event.type == SDL_QUIT) { app_running = false; return "QUIT"; }
            ui.handleEvent(event);
            handle_global_ui_keyboard(event, ui);
            if(event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    ui.onWindowResize(event.window.data1, event.window.data2);
                }
            }
            if(event.type == SDL_KEYDOWN){
                if(event.key.keysym.sym == SDLK_F11) {
                    SDL_Window* win = SDL_GetWindowFromID(event.window.windowID);
                    if(win) SDL_SetWindowFullscreen(win, (SDL_GetWindowFlags(win) & SDL_WINDOW_FULLSCREEN_DESKTOP) ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
                }
                if(event.key.keysym.sym == SDLK_UP) {
                    selection--;
                    if(selection < 0) selection = (int)games.size() - 1;
                }
                else if(event.key.keysym.sym == SDLK_DOWN) {
                    selection++;
                    if(selection >= (int)games.size()) selection = 0;
                }
                else if(event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_SPACE) {
                    if (files[selection] != "") return "LOAD:" + files[selection];
                }
                else if(event.key.keysym.sym == SDLK_ESCAPE) {
                    return "BACK";
                }
            }
        }
        
        if (selection < scroll_offset) scroll_offset = selection;
        else if (selection >= scroll_offset + max_visible) scroll_offset = selection - max_visible + 1;
        
        SDL_SetRenderTarget(renderer, crt_tex);
        SDL_SetRenderDrawColor(renderer, bg_r, bg_g, bg_b, 255);
        SDL_RenderClear(renderer);
        
        SDL_SetRenderDrawColor(renderer, fg_r, fg_g, fg_b, 255);
        retro_gui::drawPixelText(renderer, "SELECT A SAVE", CRT_WIDTH/2 - retro_gui::pixelTextWidth("SELECT A SAVE", 4)/2, 40, 4);
        
        for(int i=0; i<max_visible; i++){
            int game_idx = scroll_offset + i;
            if(game_idx >= (int)games.size()) break;
            if(game_idx == selection) retro_gui::drawPixelText(renderer, ">", 50, 120 + i*40, 3);
            retro_gui::drawPixelText(renderer, games[game_idx].c_str(), 100, 120 + i*40, 2);
        }
        retro_gui::drawPixelText(renderer, "PRESS ENTER TO LOAD", CRT_WIDTH/2 - retro_gui::pixelTextWidth("PRESS ENTER TO LOAD", 2)/2, CRT_HEIGHT - 20, 2);
        SDL_SetRenderTarget(renderer, nullptr);

        ui.beginFrame();
        ui.renderContent(crt_tex);
        ui.endFrame();
        
        SDL_RenderPresent(renderer);
        enforce_60fps(frame_start);
    }
    return "QUIT";
}

std::string run_menu(SDL_Renderer* renderer, RetroComputerUI& ui, SDL_Texture* crt_tex) {
    std::vector<std::string> games;
    std::vector<std::string> files;
    
    if (fs::exists("roms")) {
        for (const auto& entry : fs::directory_iterator("roms")) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (ext == ".ch8" || ext == ".rom") {
                    std::string name = entry.path().stem().string();
                    std::transform(name.begin(), name.end(), name.begin(), ::toupper);
                    games.push_back(name);
                    files.push_back(entry.path().generic_string());
                }
            }
        }
    }
    if (games.empty()) {
        games.push_back("NO ROMS FOUND");
        files.push_back("");
    }

    int selection = 0;
    int scroll_offset = 0;
    const int max_visible = 4;
    
    SDL_Event event;
    while(true){
        Uint64 frame_start = SDL_GetPerformanceCounter();
        while(SDL_PollEvent(&event)){
            if(event.type == SDL_QUIT) { app_running = false; return "QUIT"; }
            ui.handleEvent(event);
            handle_global_ui_keyboard(event, ui);
            if(event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    ui.onWindowResize(event.window.data1, event.window.data2);
                }
            }
            if(event.type == SDL_KEYDOWN){
                if(event.key.keysym.sym == SDLK_F11) {
                    SDL_Window* win = SDL_GetWindowFromID(event.window.windowID);
                    if(win) SDL_SetWindowFullscreen(win, (SDL_GetWindowFlags(win) & SDL_WINDOW_FULLSCREEN_DESKTOP) ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
                }
                if(event.key.keysym.sym == SDLK_UP) {
                    selection--;
                    if(selection < 0) selection = (int)games.size() - 1;
                }
                else if(event.key.keysym.sym == SDLK_DOWN) {
                    selection++;
                    if(selection >= (int)games.size()) selection = 0;
                }
                else if(event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_SPACE) {
                    if (files[selection] != "") return files[selection];
                }
                else if(event.key.keysym.sym == SDLK_ESCAPE) {
                    return "QUIT";
                }
            }
        }
        
        if (selection < scroll_offset) scroll_offset = selection;
        else if (selection >= scroll_offset + max_visible) scroll_offset = selection - max_visible + 1;
        
        SDL_SetRenderTarget(renderer, crt_tex);
        SDL_SetRenderDrawColor(renderer, bg_r, bg_g, bg_b, 255);
        SDL_RenderClear(renderer);
        
        SDL_SetRenderDrawColor(renderer, fg_r, fg_g, fg_b, 255);
        retro_gui::drawPixelText(renderer, "SELECT A GAME", CRT_WIDTH/2 - retro_gui::pixelTextWidth("SELECT A GAME", 4)/2, 40, 4);
        
        for(int i=0; i<max_visible; i++){
            int game_idx = scroll_offset + i;
            if(game_idx >= (int)games.size()) break;
            if(game_idx == selection) retro_gui::drawPixelText(renderer, ">", 150, 120 + i*40, 3);
            retro_gui::drawPixelText(renderer, games[game_idx].c_str(), 200, 120 + i*40, 3);
        }
        retro_gui::drawPixelText(renderer, "PRESS ENTER TO START", CRT_WIDTH/2 - retro_gui::pixelTextWidth("PRESS ENTER TO START", 2)/2, CRT_HEIGHT - 20, 2);
        SDL_SetRenderTarget(renderer, nullptr);

        ui.beginFrame();
        ui.renderContent(crt_tex);
        ui.endFrame();
        
        SDL_RenderPresent(renderer);
        enforce_60fps(frame_start);
    }
    return "QUIT";
}

std::string show_slots_menu(SDL_Renderer* renderer, RetroComputerUI& ui, SDL_Texture* crt_tex, const std::string& current_rom_path, bool is_save) {
    std::string base_name = current_rom_path;
    size_t slash = base_name.find_last_of("/\\");
    if(slash != std::string::npos) base_name = base_name.substr(slash + 1);
    
    std::string title = is_save ? "SAVE GAME: " : "LOAD GAME: ";
    title += base_name;
    std::transform(title.begin(), title.end(), title.begin(), ::toupper);

        std::string saves_dir = "saves/" + base_name;
    if (!std::filesystem::exists(saves_dir)) {
        std::filesystem::create_directories(saves_dir);
    }
    
    std::vector<std::string> slots(5);
    std::vector<std::string> slot_paths(5);
    for (int i=1; i<=5; i++) {
        std::string slot_file = saves_dir + "/slot" + std::to_string(i) + ".sav";
        slot_paths[i-1] = slot_file;
        if (fs::exists(slot_file)) {
            slots[i-1] = "SLOT " + std::to_string(i) + " (DATA EXISTS)";
        } else {
            slots[i-1] = "SLOT " + std::to_string(i) + " (EMPTY)";
        }
    }

    int selection = 0;
    
    SDL_Event event;
    while(true){
        Uint64 frame_start = SDL_GetPerformanceCounter();
        while(SDL_PollEvent(&event)){
            if(event.type == SDL_QUIT) { app_running = false; return "QUIT"; }
            ui.handleEvent(event);
            handle_global_ui_keyboard(event, ui);
            if(event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    ui.onWindowResize(event.window.data1, event.window.data2);
                }
            }
            if(event.type == SDL_KEYDOWN){
                if(event.key.keysym.sym == SDLK_ESCAPE || (!is_save && event.key.keysym.sym == SDLK_l) || (is_save && event.key.keysym.sym == SDLK_s)) {
                    return "BACK";
                }
                if(event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_w) {
                    selection--;
                    if(selection < 0) selection = 4;
                }
                if(event.key.keysym.sym == SDLK_DOWN || (!is_save && event.key.keysym.sym == SDLK_s)) {
                    selection++;
                    if(selection > 4) selection = 0;
                }
                if(event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_SPACE) {
                    return slot_paths[selection];
                }
            }
        }
        
        SDL_SetRenderTarget(renderer, crt_tex);
        SDL_SetRenderDrawColor(renderer, bg_r, bg_g, bg_b, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, fg_r, fg_g, fg_b, 255);
        
        SDL_Rect border = { 30, 20, CRT_WIDTH - 60, CRT_HEIGHT - 40 };
        SDL_RenderDrawRect(renderer, &border);
        border.x += 2; border.y += 2; border.w -= 4; border.h -= 4;
        SDL_RenderDrawRect(renderer, &border);
        
        retro_gui::drawPixelText(renderer, "ESC: CANCEL", CRT_WIDTH - 160, 30, 2);
        
        retro_gui::drawPixelText(renderer, title.c_str(), 
                                CRT_WIDTH/2 - retro_gui::pixelTextWidth(title.c_str(), 3)/2, 60, 3);
        
        int start_y = 120;
        for (int i=0; i<5; i++) {
            if (i == selection) {
                retro_gui::drawPixelText(renderer, ">", 60, start_y + i*25, 2);
            }
            retro_gui::drawPixelText(renderer, slots[i].c_str(), 90, start_y + i*25, 2);
        }
        
        retro_gui::drawPixelText(renderer, "PRESS ENTER TO SELECT", CRT_WIDTH/2 - retro_gui::pixelTextWidth("PRESS ENTER TO SELECT", 2)/2, 260, 2);

        SDL_SetRenderTarget(renderer, nullptr);

        ui.beginFrame();
        ui.renderContent(crt_tex);
        ui.endFrame();
        
        SDL_RenderPresent(renderer);
        
        enforce_60fps(frame_start);
    }
    return "BACK";
}

bool in_popup_menu = false;

bool show_popup(SDL_Renderer* renderer, const std::string& rom_name, RetroComputerUI& ui, SDL_Texture* crt_tex) {
    in_popup_menu = true;
    SDL_Event event;
    std::string base_name = rom_name;
    size_t slash = base_name.find_last_of("/\\");
    if(slash != std::string::npos) base_name = base_name.substr(slash + 1);
    std::transform(base_name.begin(), base_name.end(), base_name.begin(), ::toupper);

    while(true){
        Uint64 frame_start = SDL_GetPerformanceCounter();
        while(SDL_PollEvent(&event)){
            if(event.type == SDL_QUIT) { app_running = false; in_popup_menu = false; return false; }
            ui.handleEvent(event);
            handle_global_ui_keyboard(event, ui);
            if(event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    ui.onWindowResize(event.window.data1, event.window.data2);
                }
            }
            if(event.type == SDL_KEYDOWN){
                if(event.key.keysym.sym == SDLK_F11) {
                    SDL_Window* win = SDL_GetWindowFromID(event.window.windowID);
                    if(win) SDL_SetWindowFullscreen(win, (SDL_GetWindowFlags(win) & SDL_WINDOW_FULLSCREEN_DESKTOP) ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
                }
                if(event.key.keysym.sym == SDLK_ESCAPE) {
                    in_popup_menu = false;
                    return false;
                }
                if(event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_SPACE) {
                    in_popup_menu = false;
                    return true;
                }
                if(event.key.keysym.sym == SDLK_l) {
                    ui.setVirtualKeyPressed(VirtualKey::LOAD, true);
                    std::string load_res = show_slots_menu(renderer, ui, crt_tex, current_rom_path, false);
                    ui.setVirtualKeyPressed(VirtualKey::LOAD, false);
                    if (load_res != "BACK" && load_res != "QUIT") {
                        chip8.load_state(load_res);
                        in_popup_menu = false;
                        return true;
                    }
                }
            }
        }
        
        SDL_SetRenderTarget(renderer, crt_tex);
        SDL_SetRenderDrawColor(renderer, bg_r, bg_g, bg_b, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, fg_r, fg_g, fg_b, 255);
        
        SDL_Rect border = { 30, 20, CRT_WIDTH - 60, CRT_HEIGHT - 40 };
        SDL_RenderDrawRect(renderer, &border);
        border.x += 2; border.y += 2; border.w -= 4; border.h -= 4;
        SDL_RenderDrawRect(renderer, &border);
        
        retro_gui::drawPixelText(renderer, "ESC: BACK", CRT_WIDTH - 150, 30, 2);
        
        if (base_name.find("TETRIS") != std::string::npos) {
            retro_gui::drawPixelText(renderer, "- TETRIS CONTROLS -", CRT_WIDTH/2 - retro_gui::pixelTextWidth("- TETRIS CONTROLS -", 3)/2, 45, 3);
            retro_gui::drawPixelText(renderer, "UP ARROW         ROTATE", 100, 95, 2);
            retro_gui::drawPixelText(renderer, "LEFT ARROW       MOVE LEFT", 100, 125, 2);
            retro_gui::drawPixelText(renderer, "RIGHT ARROW      MOVE RIGHT", 100, 155, 2);
            retro_gui::drawPixelText(renderer, "DOWN / SPACE     DROP", 100, 185, 2);
        } else if (base_name.find("PONG") != std::string::npos) {
            retro_gui::drawPixelText(renderer, "- PONG CONTROLS -", CRT_WIDTH/2 - retro_gui::pixelTextWidth("- PONG CONTROLS -", 3)/2, 45, 3);
            retro_gui::drawPixelText(renderer, "PLAYER 1: UP / DOWN  or  W / S", 80, 105, 2);
            retro_gui::drawPixelText(renderer, "PLAYER 2: NUM8 / NUM2  or  I / K", 80, 145, 2);
        } else if (base_name.find("BLINKY") != std::string::npos) {
            retro_gui::drawPixelText(renderer, "- BLINKY (PAC-MAN) CONTROLS -", CRT_WIDTH/2 - retro_gui::pixelTextWidth("- BLINKY (PAC-MAN) CONTROLS -", 3)/2, 45, 3);
            retro_gui::drawPixelText(renderer, "ARROWS  or  WASD  : MOVE PAC-MAN", 90, 100, 2);
            retro_gui::drawPixelText(renderer, "SPACE   or  1     : START / PAUSE", 90, 135, 2);
            retro_gui::drawPixelText(renderer, "MAZE GENERATES AT START (~8 SEC)", 90, 170, 2);
        } else {
            retro_gui::drawPixelText(renderer, "- CONTROLS -", CRT_WIDTH/2 - retro_gui::pixelTextWidth("- CONTROLS -", 3)/2, 45, 3);
            retro_gui::drawPixelText(renderer, "ARROWS / 2468   MOVE", 100, 100, 2);
            retro_gui::drawPixelText(renderer, "SPACE  / 5      ACTION", 100, 135, 2);
            retro_gui::drawPixelText(renderer, "1-4,Q-R,A-F,Z-V FULL MAP", 100, 170, 2);
        }

        retro_gui::drawPixelText(renderer, "ENTER / SPACE  START GAME",    CRT_WIDTH/2 - retro_gui::pixelTextWidth("ENTER / SPACE  START GAME",    2)/2, 195, 2);
        retro_gui::drawPixelText(renderer, "L              LOAD SAVED GAME",CRT_WIDTH/2 - retro_gui::pixelTextWidth("L              LOAD SAVED GAME",2)/2, 212, 2);
        retro_gui::drawPixelText(renderer, "R              RESTART ANYTIME", CRT_WIDTH/2 - retro_gui::pixelTextWidth("R              RESTART ANYTIME", 2)/2, 229, 2);
        retro_gui::drawPixelText(renderer, "[+] FASTER  [-] SLOWER  SPEED",  CRT_WIDTH/2 - retro_gui::pixelTextWidth("[+] FASTER  [-] SLOWER  SPEED",  2)/2, 246, 2);

        SDL_SetRenderTarget(renderer, nullptr);

        ui.beginFrame();
        ui.renderContent(crt_tex);
        ui.endFrame();
        
        SDL_RenderPresent(renderer);
        enforce_60fps(frame_start);
    }
    return false;
}

// External Color Selector logic
void draw_external_color_selector(SDL_Renderer* renderer, ComputerTheme current_theme) {
    int sx = WINDOW_W - 250 - 20;
    int sy = 20;
    
    SDL_Rect bg = { sx, sy, 250, 120 };
    SDL_SetRenderDrawColor(renderer, 40, 40, 45, 255);
    SDL_RenderFillRect(renderer, &bg);
    SDL_SetRenderDrawColor(renderer, 100, 100, 110, 255);
    SDL_RenderDrawRect(renderer, &bg);
    
    SDL_SetRenderDrawColor(renderer, 200, 200, 210, 255);
    retro_gui::drawPixelText(renderer, "COMPUTER COLOR", sx + 125 - retro_gui::pixelTextWidth("COMPUTER COLOR", 2)/2, sy + 15, 2);
    
    const char* names[] = { "WHITE", "GREEN", "AMBER", "DARK" };
    ComputerTheme themes[] = { ComputerTheme::WHITE, ComputerTheme::GREEN, ComputerTheme::AMBER, ComputerTheme::CHARCOAL };
    
    for(int i=0; i<4; i++){
        int cx = sx + 25 + (i * 55);
        int cy = sy + 55;
        
        if (current_theme == themes[i]) {
            SDL_Rect hl = { cx - 5, cy - 5, 45, 55 };
            SDL_SetRenderDrawColor(renderer, 80, 80, 100, 255);
            SDL_RenderFillRect(renderer, &hl);
            SDL_SetRenderDrawColor(renderer, 200, 200, 100, 255);
            SDL_RenderDrawRect(renderer, &hl);
        }
        
        SDL_SetRenderDrawColor(renderer, 200, 200, 210, 255);
        SDL_Rect dot = { cx + 12, cy, 10, 10 };
        SDL_RenderFillRect(renderer, &dot);
        
        retro_gui::drawPixelText(renderer, names[i], cx + 17 - retro_gui::pixelTextWidth(names[i], 1)/2, cy + 20, 1);
    }
}

ComputerTheme handle_color_selector_click(int x, int y, ComputerTheme current_theme) {
    int sx = WINDOW_W - 250 - 20;
    int sy = 20;
    
    ComputerTheme themes[] = { ComputerTheme::WHITE, ComputerTheme::GREEN, ComputerTheme::AMBER, ComputerTheme::CHARCOAL };
    
    for(int i=0; i<4; i++){
        int cx = sx + 25 + (i * 55);
        int cy = sy + 55;
        SDL_Rect hl = { cx - 5, cy - 5, 45, 55 };
        SDL_Point pt = { x, y };
        if(SDL_PointInRect(&pt, &hl)){
            return themes[i];
        }
    }
    return current_theme;
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

    SDL_Window* window = SDL_CreateWindow("Chip-8 Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_W, WINDOW_H, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);

    RetroComputerUI ui;
    ui.init(renderer, WINDOW_W, WINDOW_H);
    ui.setComputerTheme(ComputerTheme::GREEN);
    
    SDL_Texture* crt_tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, CRT_WIDTH, CRT_HEIGHT);

    ui.setOnVirtualKeyPressed([](VirtualKey key, bool pressed) {
        int hex = retro_gui::virtualKeyToChip8Hex(key);
        if (hex >= 0) {
            chip8.key[hex] = pressed ? 1 : 0;
        } else if (key == VirtualKey::ARROW_UP) {
            chip8.key[2] = pressed ? 1 : 0;
        } else if (key == VirtualKey::ARROW_DOWN) {
            chip8.key[8] = pressed ? 1 : 0;
        } else if (key == VirtualKey::ARROW_LEFT) {
            chip8.key[4] = pressed ? 1 : 0;
        } else if (key == VirtualKey::ARROW_RIGHT) {
            chip8.key[6] = pressed ? 1 : 0;
        } else if (key == VirtualKey::SPACE) {
            chip8.key[5] = pressed ? 1 : 0;
        }
    });

    ui.setOnVirtualKeyReleased([](VirtualKey key, bool pressed) {
        int hex = retro_gui::virtualKeyToChip8Hex(key);
        if (hex >= 0) {
            chip8.key[hex] = pressed ? 1 : 0;
        } else if (key == VirtualKey::ARROW_UP) {
            chip8.key[2] = pressed ? 1 : 0;
        } else if (key == VirtualKey::ARROW_DOWN) {
            chip8.key[8] = pressed ? 1 : 0;
        } else if (key == VirtualKey::ARROW_LEFT) {
            chip8.key[4] = pressed ? 1 : 0;
        } else if (key == VirtualKey::ARROW_RIGHT) {
            chip8.key[6] = pressed ? 1 : 0;
        } else if (key == VirtualKey::SPACE) {
            chip8.key[5] = pressed ? 1 : 0;
        }
    });

    ui.setOnSpeedChanged([](int cycles) {
        cycles_per_frame = cycles;
    });

    ui.setOnWaveformChanged([](Waveform w) {
        current_waveform = w;
    });

    ui.setOnPaletteChanged([](PaletteID pid) {
        if (pid == PaletteID::CLASSIC_GREEN) {
            fg_r = 0x8F; fg_g = 0xEA; fg_b = 0x9B;
            bg_r = 0x0B; bg_g = 0x1F; bg_b = 0x10;
        } else if (pid == PaletteID::AMBER_CRT) {
            fg_r = 0xFF; fg_g = 0xB2; fg_b = 0x45;
            bg_r = 0x18; bg_g = 0x0D; bg_b = 0x03;
        } else if (pid == PaletteID::NEON_HIGH_CONTRAST) {
            fg_r = 0xD7; fg_g = 0xFF; fg_b = 0x3B;
            bg_r = 0x02; bg_g = 0x04; bg_b = 0x03;
        }
    });

    ui.setOnSaveStateRequested([renderer, crt_tex, &ui]() {
        if (current_rom_path != "" && !chip8.is_game_over() && !in_popup_menu) {
            ui.setVirtualKeyPressed(VirtualKey::SAVE, true);
            std::string save_res = show_slots_menu(renderer, ui, crt_tex, current_rom_path, true);
            ui.setVirtualKeyPressed(VirtualKey::SAVE, false);
            if (save_res != "BACK" && save_res != "QUIT") {
                chip8.save_state(save_res);
            }
        }
    });

    ui.setOnLoadStateRequested([renderer, crt_tex, &ui]() {
        if (current_rom_path != "" && chip8.is_game_over()) {
            ui.setVirtualKeyPressed(VirtualKey::LOAD, true);
            std::string load_res = show_slots_menu(renderer, ui, crt_tex, current_rom_path, false);
            ui.setVirtualKeyPressed(VirtualKey::LOAD, false);
            if (load_res != "BACK" && load_res != "QUIT") {
                chip8.load_state(load_res);
            }
        }
    });

    while(app_running) {
        std::string rom_to_load = "";
        if(argc >= 2) rom_to_load = argv[1];
        
        if(rom_to_load == "") {
            rom_to_load = run_menu(renderer, ui, crt_tex);
        }

        if(rom_to_load == "QUIT" || rom_to_load == "") {
            app_running = false;
            break;
        }

        bool is_loading_state = false;
        if (rom_to_load.rfind("LOAD:", 0) == 0) {
            rom_to_load = rom_to_load.substr(5);
            is_loading_state = true;
            if (rom_to_load.length() > 4) {
                current_rom_path = rom_to_load.substr(0, rom_to_load.length() - 4);
            }
        } else {
            current_rom_path = rom_to_load;
        }

        if (!show_popup(renderer, current_rom_path, ui, crt_tex)) {
            if(argc >= 2) app_running = false;
            continue;
        }

        std::string base_name = current_rom_path;
        size_t slash = base_name.find_last_of("/\\");
        if(slash != std::string::npos) base_name = base_name.substr(slash + 1);
        std::transform(base_name.begin(), base_name.end(), base_name.begin(), ::toupper);

        cycles_per_frame = ui.getSpeedValue();
        if (base_name.find("BLINKY") != std::string::npos) {
            // override speed if it's default
        }

        if (base_name.find("TETRIS") != std::string::npos || 
            base_name.find("PONG") != std::string::npos || 
            base_name.find("BLINKY") != std::string::npos ||
            base_name.find("INVADERS") != std::string::npos ||
            base_name.find("UFO") != std::string::npos ||
            base_name.find("MISSILE") != std::string::npos ||
            base_name.find("TANK") != std::string::npos) {
            chip8.quirk_shift_vy = true;
            chip8.quirk_index_increment = true;
        }
        
        if (is_loading_state) {
            chip8.load_state(rom_to_load);
        } else {
            chip8.load_rom(current_rom_path);
        }
        
        bool running = true;
        bool restart_requested = false;

        while(running && app_running){
            Uint64 frame_start = SDL_GetPerformanceCounter();
            SDL_Event event;
            while(SDL_PollEvent(&event)){
                if(event.type == SDL_QUIT) { running = false; app_running = false; }
                
                ui.handleEvent(event);
                handle_global_ui_keyboard(event, ui);
                if(event.type == SDL_WINDOWEVENT) {
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                        ui.onWindowResize(event.window.data1, event.window.data2);
                    }
                }
                
                if(event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT){
                    ui.setComputerTheme(handle_color_selector_click(event.button.x, event.button.y, ui.getComputerTheme()));
                }
                
                int map_up = 2, map_down = 8, map_left = 4, map_right = 6, map_space = 5;
                if (base_name.find("TETRIS") != std::string::npos) {
                    map_up = 4;    // Rotate (Key 4)
                    map_down = 7;  // Drop (Key 7)
                    map_left = 5;  // Move Left (Key 5)
                    map_right = 6; // Move Right (Key 6)
                    map_space = 7; // Drop
                }

                if(event.type == SDL_KEYDOWN){
                    if(event.key.keysym.sym == SDLK_F11) {
                        SDL_Window* win = SDL_GetWindowFromID(event.window.windowID);
                        if(win) SDL_SetWindowFullscreen(win, (SDL_GetWindowFlags(win) & SDL_WINDOW_FULLSCREEN_DESKTOP) ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
                    }
                    if(event.key.keysym.sym == SDLK_ESCAPE) {
                        running = false;
                    }
                    if(event.key.keysym.sym == SDLK_r) {
                        restart_requested = true;
                    }
                    if(event.key.keysym.sym == SDLK_s && !chip8.is_game_over()) {
                        // SAVE: pause and show slot picker
                        ui.setVirtualKeyPressed(VirtualKey::SAVE, true);
                        std::string save_res = show_slots_menu(renderer, ui, crt_tex, current_rom_path, true);
                        ui.setVirtualKeyPressed(VirtualKey::SAVE, false);
                        if (save_res != "BACK" && save_res != "QUIT") {
                            chip8.save_state(save_res);
                        }
                    }
                    if(event.key.keysym.sym == SDLK_l && chip8.is_game_over()) {
                        // LOAD: only available on game over screen
                        ui.setVirtualKeyPressed(VirtualKey::LOAD, true);
                        std::string load_res = show_slots_menu(renderer, ui, crt_tex, current_rom_path, false);
                        ui.setVirtualKeyPressed(VirtualKey::LOAD, false);
                        if (load_res != "BACK" && load_res != "QUIT") {
                            chip8.load_state(load_res);
                        }
                    }
                    
                    for(int i=0; i<16; i++){
                        bool is_mapped = (event.key.keysym.sym == keymap[i]);
                        if (event.key.keysym.sym == SDLK_UP && i == map_up) is_mapped = true;
                        if (event.key.keysym.sym == SDLK_DOWN && i == map_down) is_mapped = true;
                        if (event.key.keysym.sym == SDLK_LEFT && i == map_left) is_mapped = true;
                        if (event.key.keysym.sym == SDLK_RIGHT && i == map_right) is_mapped = true;
                        if (event.key.keysym.sym == SDLK_SPACE && i == map_space) is_mapped = true;
                        
                        if(is_mapped) {
                            chip8.key[i] = 1;
                        }
                    }
                }
                if(event.type == SDL_KEYUP){
                    for(int i=0; i<16; i++){
                        bool is_mapped = (event.key.keysym.sym == keymap[i]);
                        if (event.key.keysym.sym == SDLK_UP && i == map_up) is_mapped = true;
                        if (event.key.keysym.sym == SDLK_DOWN && i == map_down) is_mapped = true;
                        if (event.key.keysym.sym == SDLK_LEFT && i == map_left) is_mapped = true;
                        if (event.key.keysym.sym == SDLK_RIGHT && i == map_right) is_mapped = true;
                        if (event.key.keysym.sym == SDLK_SPACE && i == map_space) is_mapped = true;
                        
                        if(is_mapped) {
                            chip8.key[i] = 0;
                        }
                    }
                }
            }

            if(restart_requested){
                chip8.reset();
                if(is_loading_state) chip8.load_state(rom_to_load);
                else chip8.load_rom(current_rom_path);
                restart_requested = false;
                continue;
            }

            if(!chip8.is_game_over()){
                for(int i=0; i<cycles_per_frame; i++){
                    uint16_t current_pc = chip8.get_pc();
                    uint16_t opcode = (chip8.get_memory(current_pc) << 8) | chip8.get_memory(current_pc + 1);
                    
                    chip8.emulate_cycle();
                    
                    if (chip8.get_pc() == current_pc && (opcode & 0xF0FF) != 0xF00A) {
                        break;
                    }
                    if(chip8.is_game_over()) break;
                }
                chip8.update_timers();
                beeping = (chip8.get_sound_timer() > 0);
                ui.setSoundActive(beeping);
            } else {
                beeping = false;
                ui.setSoundActive(false);
            }

            SDL_SetRenderTarget(renderer, crt_tex);
            draw_graphics(renderer, chip8.display);
            if(chip8.is_game_over()){
                retro_gui::drawPixelText(renderer, "- GAME OVER -", CRT_WIDTH/2 - retro_gui::pixelTextWidth("- GAME OVER -", 4)/2, CRT_HEIGHT/2 - 30, 4);
                retro_gui::drawPixelText(renderer, "R: REPLAY     ESC: MENU", CRT_WIDTH/2 - retro_gui::pixelTextWidth("R: REPLAY     ESC: MENU", 2)/2, CRT_HEIGHT/2 + 20, 2);
                retro_gui::drawPixelText(renderer, "L: LOAD SAVED GAME", CRT_WIDTH/2 - retro_gui::pixelTextWidth("L: LOAD SAVED GAME", 2)/2, CRT_HEIGHT/2 + 42, 2);
            }
            SDL_SetRenderTarget(renderer, nullptr);

            ui.beginFrame();
            ui.renderContent(crt_tex);
            ui.endFrame();
            
            draw_external_color_selector(renderer, ui.getComputerTheme());
            
            SDL_RenderPresent(renderer);

            chip8.draw_flag = false;
            enforce_60fps(frame_start);
        }
        
        if(argc >= 2) {
            app_running = false;
        }
    }

    if(audio_device != 0) SDL_CloseAudioDevice(audio_device);
    SDL_DestroyTexture(crt_tex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}












