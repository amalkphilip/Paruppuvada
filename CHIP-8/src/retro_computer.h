#ifndef RETRO_COMPUTER_H
#define RETRO_COMPUTER_H

// ============================================================================
// RetroComputerUI — a 2.5D pixel-art "retro computer" GUI shell (SDL2 only).
//
// This module is deliberately INDEPENDENT of the CHIP-8 core. It knows
// nothing about the CPU, memory, ROMs, timers or the framebuffer. You connect
// your existing emulator through these five public interfaces:
//
//   1. Content rectangle ....... getScreenArea() / setContentRect()
//   2. Emulator rendering ...... beginFrame() / renderContent() / endFrame()
//   3. Virtual key visuals ..... setVirtualKeyPressed() / isVirtualKeyPressed()
//   4. Virtual key events ...... handleEvent() + setKeyCallback()
//   5. Palette / theme ......... setPalette() / setCustomPalette()
//
// The implementation is pure 2D SDL2 (rects, lines, scanline-filled quads and
// one texture blit). The 3D look comes from layered faces, offset shadows and
// bevel highlights — no 3D engine, no extra dependencies.
// ============================================================================

#include <SDL2/SDL.h>
#include <cstdint>

// Built-in themes.
enum class RCPalette {
    GREEN_PHOSPHOR = 0,  // default: monochrome green CRT
    AMBER,               // amber terminal
    PAPER_WHITE,         // light "paper white" terminal
    COUNT
};

struct RCColor { uint8_t r, g, b, a; };

// Full palette definition (advanced: use with setCustomPalette()).
struct RCPaletteDef {
    RCColor background;      // window background
    RCColor backgroundDots;  // halftone dot grid on the background
    RCColor bodyDark;        // side faces / deepest shadow of the case
    RCColor bodyMid;         // front face of the case
    RCColor bodyLight;       // top face / lit surfaces
    RCColor bodyEdge;        // thin highlight lines along lit edges
    RCColor bezel;           // CRT bezel plastic
    RCColor bezelDark;       // recess shadow inside the bezel
    RCColor glass;           // unlit CRT glass
    RCColor screenGlow;      // phosphor glow tint around the screen
    RCColor keyFace;         // keypad button top face
    RCColor keyTop;          // keypad button lit edge
    RCColor keySide;         // keypad button side/extrusion
    RCColor keyLabel;        // keypad glyph colour
    RCColor accent;          // power LED, glow, small accents
    RCColor textMain;        // GUI text (brand plate etc.)
};

// Callback for virtual keypad events.
//   key:     0x0-0xF (the CHIP-8 key index, keypad order 123C/456D/789E/A0BF)
//   pressed: true on press, false on release
// Connect this to your emulator's key[] array (or anything else) yourself.
typedef void (*RCKeyCallback)(int key, bool pressed, void* userdata);

class RetroComputerUI {
public:
    RetroComputerUI();
    ~RetroComputerUI();

    // Call once after creating your SDL_Renderer.
    // windowW/windowH: the drawable size of your window.
    bool init(SDL_Renderer* renderer, int windowW, int windowH);
    void shutdown();

    // ------------------------------------------------------------------
    // Interface 1: the CRT content rectangle
    // ------------------------------------------------------------------
    // Where your emulator content (menu / popup / 64x32 framebuffer) goes,
    // in window pixels. Use it to size/position whatever you render.
    SDL_Rect getScreenArea() const;
    // Optional override if you want full manual control of the CRT area.
    void     setContentRect(SDL_Rect rect);

    // ------------------------------------------------------------------
    // Interface 2: frame pipeline — render your emulator into the CRT
    // ------------------------------------------------------------------
    // Typical frame:
    //     ui.beginFrame();                 // background + computer + bezel
    //     ui.renderContent(contentTexture);// your existing UI, inside the CRT
    //     ui.endFrame();                   // CRT effects + keypad + present
    //
    // `content` is any SDL_Texture holding your existing emulator UI
    // (e.g. a 640x320 SDL_TEXTUREACCESS_TARGET texture you rendered your
    // current menu/popup/framebuffer into). It is blitted into the CRT
    // screen area with nearest-neighbour scaling. Passing nullptr draws an
    // unlit screen.
    void beginFrame();
    void renderContent(SDL_Texture* content);
    void endFrame();

    // ------------------------------------------------------------------
    // Interface 3: virtual keypad visuals
    // ------------------------------------------------------------------
    // key is the CHIP-8 key index 0x0-0xF. Purely visual: the GUI never
    // touches emulator state.
    void setVirtualKeyPressed(int key, bool pressed);
    bool isVirtualKeyPressed(int key) const;

    // ------------------------------------------------------------------
    // Interface 4: virtual keypad events
    // ------------------------------------------------------------------
    // Feed your SDL events here. Mouse press/release on a virtual key
    // updates its visual state AND fires the callback (if set).
    // SDL_QUIT etc. are ignored — your own event loop keeps ownership.
    void handleEvent(const SDL_Event& event);
    void setKeyCallback(RCKeyCallback cb, void* userdata);
    void setSpeedLevel(int speed);

    // ------------------------------------------------------------------
    // Interface 5: palette / theme
    // ------------------------------------------------------------------
    void      setPalette(RCPalette palette);
    void      setCustomPalette(const RCPaletteDef& def);
    RCPalette getPalette() const;
    static RCPaletteDef builtinPalette(RCPalette id);

    // Call if your window is resizable (optional; demo uses a fixed window).
    void onWindowResize(int w, int h);

private:
    struct Layout {
        SDL_Rect  body;              // monitor front face
        int       depthX, depthY;    // fake-3D case extrusion (right + top)
        SDL_Rect  bezel;             // CRT bezel
        SDL_Rect  glass;             // glass incl. dark border
        SDL_Rect  screen;            // === THE CRT CONTENT AREA ===
        SDL_Point deck[4];           // keyboard deck trapezoid (back L/R, front R/L)
        int       deckFaceH;         // deck front thickness
        SDL_Rect  key[20];           // top face of each key, indexed 0x0-0xF, 16=SPD-, 17=SPD+, 18=SAVE, 19=LOAD
        int       keyDepth;          // key extrusion height
        SDL_Rect  brandPlate;        // decorative brand plate
        SDL_Rect  cartSlot;          // decorative cartridge slot
        SDL_Rect  powerLed;          // decorative power LED
        SDL_Rect  speedBar;          // speed indicator bar
    };

    // layout
    void computeLayout(int w, int h);

    // drawing stages
    void drawBackground();
    void drawGroundShadow();
    void drawComputerBody();
    void drawBezelAndGlass();
    void drawCrtEffects();
    void drawDecorations();
    void drawKeypad();
    void drawKey(int key);

    // primitives
    void setColor(RCColor c) const;
    void setColor(RCColor c, uint8_t alphaOverride) const;
    void fillQuad(int x0, int y0, int x1, int y1,
                  int x2, int y2, int x3, int y3) const;
    void fillPoly(const SDL_Point* pts, int count) const;
    void thickLine(int x0, int y0, int x1, int y1, int thickness) const;
    static SDL_Rect inflate(SDL_Rect r, int d);

    // text (3x5 pixel font, same style as the existing emulator UI)
    void drawText(const char* s, int x, int y, int scale, RCColor c) const;

    int keyAtPoint(int x, int y) const;   // mouse hit test -> 0x0-0xF or -1

    SDL_Renderer* ren_;
    int   winW_, winH_;
    float scale_;       // design-space (960x840) -> window scale
    int   originX_, originY_;

    Layout       lay_;
    RCPaletteDef pal_;
    RCPalette    palId_;
    bool         screenOverride_;

    bool keyPressed_[20];
    uint32_t keyFlashStart_[20]; // Added for non-blocking blink effect
    int  mouseKey_;     // key currently held by the mouse, or -1
    int  speedLevel_;   // current emulation speed for light bar

    RCKeyCallback cb_;
    void*         cbUser_;
};

// ----------------------------------------------------------------------------
// Shared 3x5 pixel-font helpers (uppercase A-Z, 0-9 and a few symbols).
// These are the same style the existing emulator UI uses, exposed so your
// menu/popup code and the GUI demo can draw matching text anywhere.
// ----------------------------------------------------------------------------
namespace rcui {
    void drawText(SDL_Renderer* renderer, const char* text, int x, int y, int scale);
    int  textWidth(const char* text, int scale);
}

#endif // RETRO_COMPUTER_H
