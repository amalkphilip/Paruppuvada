#include "chip8.h"
#include "retro_computer.h"
#include <SDL2/SDL.h>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

// Virtual CRT Resolution for CHIP-8 native scaling
const int CRT_WIDTH = 640;
const int CRT_HEIGHT = 320;

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
    float scale_x = (float)CRT_WIDTH / 64.0f;
    float scale_y = (float)CRT_HEIGHT / 32.0f;

    SDL_SetRenderDrawColor(renderer, 8, 22, 10, 255); // Dark CRT background
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 0x8F, 0xEA, 0x9B, 255); // Green phosphor pixels
    for(int y=0; y<32; y++){
        for(int x=0; x<64; x++){
            if(display[x + (y*64)] == 1){
                SDL_Rect rect = { (int)(x*scale_x), (int)(y*scale_y), (int)scale_x + 1, (int)scale_y + 1 };
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }
}

int cycles_per_frame = 10;
std::string current_rom_path = "";

void handle_speed_keys(const SDL_Event& event, RetroComputerUI& ui) {
    if(event.type == SDL_KEYDOWN){
        if(event.key.keysym.sym == SDLK_PLUS || event.key.keysym.sym == SDLK_KP_PLUS || event.key.keysym.sym == SDLK_EQUALS) {
            cycles_per_frame += 1;
            ui.setVirtualKeyPressed(17, true);
        } else if(event.key.keysym.sym == SDLK_MINUS || event.key.keysym.sym == SDLK_KP_MINUS) {
            cycles_per_frame = std::max(1, cycles_per_frame - 1);
            ui.setVirtualKeyPressed(16, true);
        }
    }
    if(event.type == SDL_KEYUP){
        if(event.key.keysym.sym == SDLK_PLUS || event.key.keysym.sym == SDLK_KP_PLUS || event.key.keysym.sym == SDLK_EQUALS) ui.setVirtualKeyPressed(17, false);
        if(event.key.keysym.sym == SDLK_MINUS || event.key.keysym.sym == SDLK_KP_MINUS) ui.setVirtualKeyPressed(16, false);
    }
}

void handle_input(Chip8& chip8, bool& running, RetroComputerUI& ui){
    SDL_Event event;
    while(SDL_PollEvent(&event)){
        ui.handleEvent(event); // GUI handles virtual mouse clicks
        handle_speed_keys(event, ui);
        
        if(event.type == SDL_QUIT) running = false;
        if(event.type == SDL_KEYDOWN){
            if(event.key.keysym.sym == SDLK_ESCAPE) running = false;
            if(event.key.keysym.sym == SDLK_F5 || event.key.keysym.sym == SDLK_s) ui.setVirtualKeyPressed(18, true);
            if(event.key.keysym.sym == SDLK_F9 || event.key.keysym.sym == SDLK_l) ui.setVirtualKeyPressed(19, true);
            
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
            if(event.key.keysym.sym == SDLK_F5 || event.key.keysym.sym == SDLK_s) ui.setVirtualKeyPressed(18, false);
            if(event.key.keysym.sym == SDLK_F9 || event.key.keysym.sym == SDLK_l) ui.setVirtualKeyPressed(19, false);
            
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
    bool selecting = true;
    while(selecting){
        while(SDL_PollEvent(&event)){
            ui.handleEvent(event);
            handle_speed_keys(event, ui);
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
        SDL_SetRenderDrawColor(renderer, 8, 22, 10, 255);
        SDL_RenderClear(renderer);
        
        SDL_SetRenderDrawColor(renderer, 0x8F, 0xEA, 0x9B, 255);
        rcui::drawText(renderer, "SELECT A SAVE", CRT_WIDTH/2 - (13*16)/2, 40, 4);
        
        for(int i=0; i<max_visible; i++){
            int game_idx = scroll_offset + i;
            if(game_idx >= (int)games.size()) break;
            if(game_idx == selection) rcui::drawText(renderer, ">", 50, 120 + i*40, 3);
            rcui::drawText(renderer, games[game_idx].c_str(), 100, 120 + i*40, 2);
        }
        rcui::drawText(renderer, "PRESS ENTER TO LOAD", CRT_WIDTH/2 - (19*8)/2, CRT_HEIGHT - 40, 2);
        SDL_SetRenderTarget(renderer, nullptr);

        ui.setSpeedLevel(cycles_per_frame);
        ui.beginFrame();
        ui.renderContent(crt_tex);
        ui.endFrame();
        
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
    bool selecting = true;
    while(selecting){
        while(SDL_PollEvent(&event)){
            ui.handleEvent(event);
            handle_speed_keys(event, ui);
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
                    if (files[selection] != "") return files[selection];
                }
                else if(event.key.keysym.sym == SDLK_ESCAPE) {
                    return "QUIT";
                }
            }
            if (ui.isVirtualKeyPressed(19)) {
                std::string load_res = show_load_menu(renderer, ui, crt_tex);
                if (load_res != "BACK" && load_res != "QUIT") return load_res;
                if (load_res == "QUIT") return "QUIT";
            }
        }
        
        if (selection < scroll_offset) scroll_offset = selection;
        else if (selection >= scroll_offset + max_visible) scroll_offset = selection - max_visible + 1;
        
        // Render to CRT texture
        SDL_SetRenderTarget(renderer, crt_tex);
        SDL_SetRenderDrawColor(renderer, 8, 22, 10, 255); // Dark CRT background
        SDL_RenderClear(renderer);
        
        SDL_SetRenderDrawColor(renderer, 0x8F, 0xEA, 0x9B, 255); // Bright green phosphor text
        rcui::drawText(renderer, "SELECT A GAME", CRT_WIDTH/2 - (13*16)/2, 40, 4);
        
        for(int i=0; i<max_visible; i++){
            int game_idx = scroll_offset + i;
            if(game_idx >= (int)games.size()) break;
            if(game_idx == selection) rcui::drawText(renderer, ">", 150, 120 + i*40, 3);
            rcui::drawText(renderer, games[game_idx].c_str(), 200, 120 + i*40, 3);
        }
        rcui::drawText(renderer, "PRESS ENTER TO START", CRT_WIDTH/2 - (20*8)/2, CRT_HEIGHT - 40, 2);
        SDL_SetRenderTarget(renderer, nullptr);

        // Render whole UI
        ui.setSpeedLevel(cycles_per_frame);
        ui.beginFrame();
        ui.renderContent(crt_tex);
        ui.endFrame();
        
        SDL_Delay(16);
    }
    return "QUIT";
}

bool show_popup(SDL_Renderer* renderer, const std::string& rom_name, RetroComputerUI& ui, SDL_Texture* crt_tex) {
    SDL_Event event;
    bool in_popup = true;
    
    std::string base_name = rom_name;
    size_t slash = base_name.find_last_of("/\\");
    if(slash != std::string::npos) base_name = base_name.substr(slash + 1);
    std::transform(base_name.begin(), base_name.end(), base_name.begin(), ::toupper);

    while(in_popup){
        while(SDL_PollEvent(&event)){
            ui.handleEvent(event);
            handle_speed_keys(event, ui);
            if(event.type == SDL_QUIT) return false;
            if(event.type == SDL_KEYDOWN){
                if(event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_SPACE) return true;
            }
        }
        
        SDL_SetRenderTarget(renderer, crt_tex);
        SDL_SetRenderDrawColor(renderer, 8, 22, 10, 255); // Dark CRT background
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 0x8F, 0xEA, 0x9B, 255); // Bright green phosphor text
        
        SDL_Rect border = { 50, 30, CRT_WIDTH - 100, CRT_HEIGHT - 60 };
        SDL_RenderDrawRect(renderer, &border);
        border.x += 2; border.y += 2; border.w -= 4; border.h -= 4;
        SDL_RenderDrawRect(renderer, &border);
        
        rcui::drawText(renderer, "[X] ESC TO CLOSE", CRT_WIDTH - 200, 40, 2);
        
        if (base_name.find("TETRIS") != std::string::npos) {
            rcui::drawText(renderer, "- TETRIS CONTROLS -", CRT_WIDTH/2 - (19*12)/2, 70, 3);
            rcui::drawText(renderer, "W   ROTATE", 150, 140, 3);
            rcui::drawText(renderer, "A   LEFT", 150, 180, 3);
            rcui::drawText(renderer, "D   RIGHT", 350, 140, 3);
            rcui::drawText(renderer, "S   DROP", 350, 180, 3);
        } else if (base_name.find("PONG") != std::string::npos) {
            rcui::drawText(renderer, "- PONG CONTROLS -", CRT_WIDTH/2 - (17*12)/2, 70, 3);
            rcui::drawText(renderer, "PLAYER 1:", 100, 130, 3);
            rcui::drawText(renderer, "UP   DOWN", 100, 170, 2);
            rcui::drawText(renderer, "ARROW KEYS", 100, 200, 2);
            rcui::drawText(renderer, "PLAYER 2:", 350, 130, 3);
            rcui::drawText(renderer, "NUM8 NUM2", 350, 170, 2);
        } else if (base_name.find("BLINKY") != std::string::npos) {
            rcui::drawText(renderer, "- BLINKY CONTROLS -", CRT_WIDTH/2 - (19*12)/2, 70, 3);
            rcui::drawText(renderer, "USE THE", 200, 140, 4);
            rcui::drawText(renderer, "ARROW KEYS", 200, 200, 4);
        } else {
            rcui::drawText(renderer, "- DEFAULT CONTROLS -", CRT_WIDTH/2 - (20*12)/2, 50, 3);
            rcui::drawText(renderer, "ARROWS / 2468   MOVE", 80, 100, 3);
            rcui::drawText(renderer, "SPACE  / 5      ACTION", 80, 140, 3);
            rcui::drawText(renderer, "+ / -           SPEED", 80, 180, 3);
            rcui::drawText(renderer, "1-4,Q-R,A-F,Z-V FULL MAP", 80, 220, 2);
        }
        SDL_SetRenderTarget(renderer, nullptr);

        ui.setSpeedLevel(cycles_per_frame);
        ui.beginFrame();
        ui.renderContent(crt_tex);
        ui.endFrame();
        SDL_Delay(16);
    }
    return false;
}

void on_virtual_key(int key, bool pressed, void* userdata) {
    Chip8* chip8 = (Chip8*)userdata;
    if (key < 16) {
        if (chip8) chip8->key[key] = pressed ? 1 : 0;
    } else if (pressed) {
        if (key == 16) cycles_per_frame = std::max(1, cycles_per_frame - 1);
        if (key == 17) cycles_per_frame += 1;
        if (key == 18) {
            if (chip8 && current_rom_path != "") {
                chip8->save_state(current_rom_path + ".sav");
                std::cout << "Saved state to " << current_rom_path << ".sav" << std::endl;
            }
        }
    }
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

    SDL_Window* window = SDL_CreateWindow("Chip-8 Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 960, 840, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);

    RetroComputerUI ui;
    ui.init(renderer, 960, 840);
    ui.setKeyCallback(on_virtual_key, nullptr); // Set early so menu clicks work
    
    SDL_Texture* crt_tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, CRT_WIDTH, CRT_HEIGHT);

    bool app_running = true;
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
            app_running = false;
            break; 
        }

        keymap[0] = SDLK_x; keymap[1] = SDLK_1; keymap[2] = SDLK_2; keymap[3] = SDLK_3;
        keymap[4] = SDLK_q; keymap[5] = SDLK_w; keymap[6] = SDLK_e; keymap[7] = SDLK_a;
        keymap[8] = SDLK_s; keymap[9] = SDLK_d; keymap[10] = SDLK_z; keymap[11] = SDLK_c;
        keymap[12] = SDLK_4; keymap[13] = SDLK_r; keymap[14] = SDLK_f; keymap[15] = SDLK_v;

        std::string base_name = rom_to_load;
        size_t slash = base_name.find_last_of("/\\");
        if(slash != std::string::npos) base_name = base_name.substr(slash + 1);
        std::transform(base_name.begin(), base_name.end(), base_name.begin(), ::toupper);

        if (base_name.find("TETRIS") != std::string::npos) {
            keymap[5] = SDLK_w; keymap[4] = SDLK_a; keymap[6] = SDLK_d; keymap[7] = SDLK_s;
        } else if (base_name.find("PONG") != std::string::npos) {
            keymap[1] = SDLK_UP; keymap[4] = SDLK_DOWN; keymap[0xC] = SDLK_KP_8; keymap[0xD] = SDLK_KP_2;
        } else if (base_name.find("BLINKY") != std::string::npos) {
            keymap[3] = SDLK_UP; keymap[6] = SDLK_DOWN; keymap[7] = SDLK_LEFT; keymap[8] = SDLK_RIGHT;
        }

        Chip8 chip8;
        
        // Classic games often rely on original VIP hardware quirks
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
        
        ui.setKeyCallback(on_virtual_key, &chip8);
        
        bool running = true;
        while(running){
            handle_input(chip8, running, ui);
            for(int i=0; i<cycles_per_frame; i++){
                uint16_t current_pc = chip8.get_pc();
                uint16_t opcode = (chip8.get_memory(current_pc) << 8) | chip8.get_memory(current_pc + 1);
                
                chip8.emulate_cycle();
                
                // Classic CHIP-8 games end by entering an infinite jump-to-self loop.
                // If PC didn't advance and we aren't waiting for a key press (Fx0A), the game is over.
                if (chip8.get_pc() == current_pc && (opcode & 0xF0FF) != 0xF00A) {
                    running = false;
                    break;
                }
            }
            chip8.update_timers();
            beeping = (chip8.get_sound_timer() > 0);

            SDL_SetRenderTarget(renderer, crt_tex);
            draw_graphics(renderer, chip8.display);
            SDL_SetRenderTarget(renderer, nullptr);

            ui.setSpeedLevel(cycles_per_frame);
            ui.beginFrame();
            ui.renderContent(crt_tex);
            ui.endFrame();

            chip8.draw_flag = false;
            SDL_Delay(16); // ~60 FPS
        }
        
        ui.setKeyCallback(on_virtual_key, nullptr); // reset to nullptr for menu
        
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