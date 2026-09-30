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

const int CRT_WIDTH = 640;
const int CRT_HEIGHT = 320;
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

// Minimal 3x5 font for simple text rendering
const uint16_t tiny_font[128] = {
    0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
    0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
    0x0000,0x2092,0x5200,0x72f2,0x2722,0x4210,0x25a5,0x2000,0x2222,0x4444,0x0525,0x0272,0x0024,0x0070,0x0002,0x1248,
    0x7557,0x2222,0x7174,0x7171,0x5571,0x7471,0x7477,0x7111,0x7777,0x7771,0x0202,0x0204,0x1242,0x0707,0x4212,0x7120,
    0x2552,0x2575,0x7577,0x3443,0x6556,0x7464,0x7464,0x3453,0x5575,0x7222,0x3113,0x5645,0x4447,0x5755,0x5755,0x2552,
    0x7574,0x2553,0x7565,0x3436,0x7222,0x5552,0x5522,0x5555,0x5225,0x5522,0x7127,0x6226,0x4210,0x3443,0x0200,0x0007,
    0x2000,0x2575,0x7577,0x3443,0x6556,0x7464,0x7464,0x3453,0x5575,0x7222,0x3113,0x5645,0x4447,0x5755,0x5755,0x2552,
    0x7574,0x2553,0x7565,0x3436,0x7222,0x5552,0x5522,0x5555,0x5225,0x5522,0x7127,0x3223,0x2222,0x6446,0x0000,0x0000
};

void drawText(SDL_Renderer* renderer, const std::string& text, int x, int y, int scale) {
    int cur_x = x;
    for(char c : text){
        if(c >= 0 && c < 128){
            uint16_t glyph = tiny_font[(int)c];
            for(int row=0; row<5; row++){
                for(int col=0; col<3; col++){
                    if(glyph & (1 << (14 - (row*3 + col)))){
                        SDL_Rect r = { cur_x + col*scale, y + row*scale, scale, scale };
                        SDL_RenderFillRect(renderer, &r);
                    }
                }
            }
        }
        cur_x += 4 * scale;
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
                
                // Crisp core pixel with a small 1-pixel gap to prevent smearing
                SDL_SetRenderDrawColor(renderer, fg_r, fg_g, fg_b, 255);
                SDL_Rect rect = { px, py, pw - 1, ph - 1 };
                SDL_RenderFillRect(renderer, &rect);
                
                // Very subtle outer glow
                SDL_SetRenderDrawColor(renderer, fg_r, fg_g, fg_b, 40);
                SDL_Rect g_rect = { px - 1, py - 1, pw + 1, ph + 1 };
                SDL_RenderFillRect(renderer, &g_rect);
            }
        }
    }
}

int cycles_per_frame = 12;
std::string current_rom_path = "";
bool app_running = true;

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
        while(SDL_PollEvent(&event)){
            if(event.type == SDL_QUIT) { app_running = false; return "QUIT"; }
            ui.handleEvent(event);
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
        drawText(renderer, "SELECT A SAVE", CRT_WIDTH/2 - (13*16)/2, 40, 4);
        
        for(int i=0; i<max_visible; i++){
            int game_idx = scroll_offset + i;
            if(game_idx >= (int)games.size()) break;
            if(game_idx == selection) drawText(renderer, ">", 50, 120 + i*40, 3);
            drawText(renderer, games[game_idx].c_str(), 100, 120 + i*40, 2);
        }
        drawText(renderer, "PRESS ENTER TO LOAD", CRT_WIDTH/2 - (19*8)/2, CRT_HEIGHT - 40, 2);
        SDL_SetRenderTarget(renderer, nullptr);

        ui.beginFrame();
        ui.renderContent(crt_tex);
        ui.endFrame();
        
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
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
        while(SDL_PollEvent(&event)){
            if(event.type == SDL_QUIT) { app_running = false; return "QUIT"; }
            ui.handleEvent(event);
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
        drawText(renderer, "SELECT A GAME", CRT_WIDTH/2 - (13*16)/2, 40, 4);
        
        for(int i=0; i<max_visible; i++){
            int game_idx = scroll_offset + i;
            if(game_idx >= (int)games.size()) break;
            if(game_idx == selection) drawText(renderer, ">", 150, 120 + i*40, 3);
            drawText(renderer, games[game_idx].c_str(), 200, 120 + i*40, 3);
        }
        drawText(renderer, "PRESS ENTER TO START", CRT_WIDTH/2 - (20*8)/2, CRT_HEIGHT - 40, 2);
        SDL_SetRenderTarget(renderer, nullptr);

        ui.beginFrame();
        ui.renderContent(crt_tex);
        ui.endFrame();
        
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }
    return "QUIT";
}

bool show_popup(SDL_Renderer* renderer, const std::string& rom_name, RetroComputerUI& ui, SDL_Texture* crt_tex) {
    SDL_Event event;
    std::string base_name = rom_name;
    size_t slash = base_name.find_last_of("/\\");
    if(slash != std::string::npos) base_name = base_name.substr(slash + 1);
    std::transform(base_name.begin(), base_name.end(), base_name.begin(), ::toupper);

    while(true){
        while(SDL_PollEvent(&event)){
            if(event.type == SDL_QUIT) { app_running = false; return false; }
            ui.handleEvent(event);
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
                    return false;
                }
                if(event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_SPACE) {
                    return true;
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
        
        drawText(renderer, "ESC: BACK", CRT_WIDTH - 150, 30, 2);
        
        if (base_name.find("TETRIS") != std::string::npos) {
            drawText(renderer, "- TETRIS CONTROLS -", CRT_WIDTH/2 - (19*12)/2, 45, 3);
            drawText(renderer, "W / UP ARROW     ROTATE", 100, 95, 2);
            drawText(renderer, "A / LEFT ARROW   MOVE LEFT", 100, 125, 2);
            drawText(renderer, "D / RIGHT ARROW  MOVE RIGHT", 100, 155, 2);
            drawText(renderer, "S / DOWN / SPACE DROP", 100, 185, 2);
        } else if (base_name.find("PONG") != std::string::npos) {
            drawText(renderer, "- PONG CONTROLS -", CRT_WIDTH/2 - (17*12)/2, 45, 3);
            drawText(renderer, "PLAYER 1: UP / DOWN  or  W / S", 80, 105, 2);
            drawText(renderer, "PLAYER 2: NUM8 / NUM2  or  I / K", 80, 145, 2);
        } else if (base_name.find("BLINKY") != std::string::npos) {
            drawText(renderer, "- BLINKY (PAC-MAN) CONTROLS -", CRT_WIDTH/2 - (29*12)/2, 45, 3);
            drawText(renderer, "ARROWS  or  WASD  : MOVE PAC-MAN", 90, 100, 2);
            drawText(renderer, "SPACE   or  1     : START / PAUSE", 90, 135, 2);
            drawText(renderer, "MAZE GENERATES AT START (~8 SEC)", 90, 170, 2);
        } else {
            drawText(renderer, "- CONTROLS -", CRT_WIDTH/2 - (12*12)/2, 45, 3);
            drawText(renderer, "ARROWS / 2468   MOVE", 100, 100, 2);
            drawText(renderer, "SPACE  / 5      ACTION", 100, 135, 2);
            drawText(renderer, "1-4,Q-R,A-F,Z-V FULL MAP", 100, 170, 2);
        }

        drawText(renderer, "R: RESTART GAME AT ANY TIME", CRT_WIDTH/2 - (27*8)/2, 215, 2);
        drawText(renderer, "SPEED: [+] FASTER   [-] SLOWER", CRT_WIDTH/2 - (30*8)/2, 240, 2);
        drawText(renderer, "PRESS ENTER OR SPACE TO START", CRT_WIDTH/2 - (29*8)/2, 270, 2);

        SDL_SetRenderTarget(renderer, nullptr);

        ui.beginFrame();
        ui.renderContent(crt_tex);
        ui.endFrame();
        
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
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
    drawText(renderer, "COMPUTER COLOR", sx + 125 - (14*8)/2, sy + 15, 2);
    
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
        
        drawText(renderer, names[i], cx + 17 - ((int)strlen(names[i])*4)/2, cy + 20, 1);
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

    Chip8 chip8;

    ui.setOnVirtualKeyPressed([&chip8](VirtualKey key, bool pressed) {
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

    ui.setOnVirtualKeyReleased([&chip8](VirtualKey key, bool pressed) {
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

    ui.setOnSaveStateRequested([&chip8]() {
        if (current_rom_path != "") {
            chip8.save_state(current_rom_path + ".sav");
            std::cout << "Saved state to " << current_rom_path << ".sav" << std::endl;
        }
    });

    ui.setOnLoadStateRequested([&chip8]() {
        // Just dummy logic for now to allow external load menu to trigger
        // Actual loading is handled globally or via load menu if available
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
            SDL_Event event;
            while(SDL_PollEvent(&event)){
                if(event.type == SDL_QUIT) { running = false; app_running = false; }
                
                ui.handleEvent(event); 
                if(event.type == SDL_WINDOWEVENT) {
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                        ui.onWindowResize(event.window.data1, event.window.data2);
                    }
                }
                
                if(event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT){
                    ui.setComputerTheme(handle_color_selector_click(event.button.x, event.button.y, ui.getComputerTheme()));
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
                    if(event.key.keysym.sym == SDLK_F5) {
                        if (current_rom_path != "") chip8.save_state(current_rom_path + ".sav");
                    }
                    if(event.key.keysym.sym == SDLK_F9) {
                        // LOAD popup
                        std::string load_res = show_load_menu(renderer, ui, crt_tex);
                        if (load_res != "BACK" && load_res != "QUIT") {
                            is_loading_state = true;
                            rom_to_load = load_res.substr(5);
                            restart_requested = true;
                        }
                        if (load_res == "QUIT") { app_running = false; running = false; }
                    }
                    
                    for(int i=0; i<16; i++){
                        bool is_mapped = (event.key.keysym.sym == keymap[i]);
                        if (event.key.keysym.sym == SDLK_UP && i == 2) is_mapped = true;
                        if (event.key.keysym.sym == SDLK_DOWN && i == 8) is_mapped = true;
                        if (event.key.keysym.sym == SDLK_LEFT && i == 4) is_mapped = true;
                        if (event.key.keysym.sym == SDLK_RIGHT && i == 6) is_mapped = true;
                        if (event.key.keysym.sym == SDLK_SPACE && i == 5) is_mapped = true;
                        
                        if(is_mapped) {
                            chip8.key[i] = 1;
                            ui.setVirtualKeyPressed(i, true);
                        }
                    }
                }
                if(event.type == SDL_KEYUP){
                    for(int i=0; i<16; i++){
                        bool is_mapped = (event.key.keysym.sym == keymap[i]);
                        if (event.key.keysym.sym == SDLK_UP && i == 2) is_mapped = true;
                        if (event.key.keysym.sym == SDLK_DOWN && i == 8) is_mapped = true;
                        if (event.key.keysym.sym == SDLK_LEFT && i == 4) is_mapped = true;
                        if (event.key.keysym.sym == SDLK_RIGHT && i == 6) is_mapped = true;
                        if (event.key.keysym.sym == SDLK_SPACE && i == 5) is_mapped = true;
                        
                        if(is_mapped) {
                            chip8.key[i] = 0;
                            ui.setVirtualKeyPressed(i, false);
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
                drawText(renderer, "- GAME OVER -", CRT_WIDTH/2 - (13*16)/2, CRT_HEIGHT/2 - 20, 4);
                drawText(renderer, "PRESS R TO REPLAY  ESC TO MENU", CRT_WIDTH/2 - (30*8)/2, CRT_HEIGHT/2 + 30, 2);
            }
            SDL_SetRenderTarget(renderer, nullptr);

            ui.beginFrame();
            ui.renderContent(crt_tex);
            ui.endFrame();
            
            draw_external_color_selector(renderer, ui.getComputerTheme());
            
            SDL_RenderPresent(renderer);

            chip8.draw_flag = false;
            SDL_Delay(16);
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
