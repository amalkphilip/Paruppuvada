#ifndef RETRO_GUI_RETRO_COMPUTER_UI_H
#define RETRO_GUI_RETRO_COMPUTER_UI_H

// ============================================================================
// RetroComputerUI — standalone SDL2 retro-computer GUI package.
//
// GUI responsibility:
//   * physical white/green computer rendering
//   * recessed primary CRT and its content rectangle
//   * upper CPU SPEED / DISPLAY PALETTE knobs
//   * recessed animated waveform instrument panel
//   * SOUND waveform knob
//   * SAVE / LOAD controls
//   * CHIP-8 hexadecimal keypad, arrows and SPACE
//   * non-blocking visual press feedback
//   * GUI-side callbacks for host integration
//
// Host/emulator responsibility:
//   * CHIP-8 CPU, memory, timers, ROM loading, game input semantics
//   * actual sound generation, savestate serialization and emulator timing
// ============================================================================

#include <SDL2/SDL.h>

#include "retro_theme.h"
#include "virtual_key.h"
#include "waveform.h"

#include <array>
#include <cstdint>
#include <functional>

namespace retro_gui {

class RetroComputerUI {
public:
    using VirtualKeyCallback = std::function<void(VirtualKey key, bool pressed)>;
    using SpeedCallback = std::function<void(int cyclesPerFrame)>;
    using PaletteCallback = std::function<void(PaletteID palette)>;
    using ComputerThemeCallback = std::function<void(ComputerTheme theme)>;
    using ActionCallback = std::function<void()>;
    using WaveformCallback = std::function<void(Waveform waveform)>;

    RetroComputerUI();
    ~RetroComputerUI();

    bool init(SDL_Renderer* renderer, int windowW, int windowH);
    void shutdown();

    // Rendering pipeline. Render host/emulator content into an
    // SDL_TEXTUREACCESS_TARGET texture, then pass it to renderContent().
    void beginFrame();
    void renderContent(SDL_Texture* content);
    void endFrame();

    // Convenience full-frame render. This draws the GUI with an unlit CRT and
    // presents the frame. Integration normally uses the three-step pipeline.
    void render(SDL_Renderer* renderer);

    // The CRT viewport in window pixels. The host renders its existing CHIP-8
    // menu/framebuffer into this rectangle.
    SDL_Rect getCRTContentRect() const;
    SDL_Rect getScreenArea() const { return getCRTContentRect(); }
    void setContentRect(SDL_Rect rect);

    // Event routing. The GUI consumes clicks on its virtual controls and emits
    // callbacks; ordinary keyboard/game mapping remains host-owned.
    void handleEvent(const SDL_Event& event);
    void onWindowResize(int w, int h);

    // Visual state and generic virtual-key events.
    void setVirtualKeyPressed(VirtualKey key, bool pressed);
    void setVirtualKeyPressed(int chip8HexKey, bool pressed);
    bool isVirtualKeyPressed(VirtualKey key) const;
    bool isVirtualKeyPressed(int chip8HexKey) const;

    // Independent GUI state.
    void setComputerTheme(ComputerTheme theme);
    ComputerTheme getComputerTheme() const;

    void setDisplayPalette(PaletteID palette);
    PaletteID getDisplayPalette() const;

    void setWaveform(Waveform waveform);
    Waveform getWaveform() const;

    void setSpeedIndex(int index);
    int getSpeedIndex() const;
    int getSpeedValue() const;

    void setSoundActive(bool active);
    bool isSoundActive() const;

    // Callbacks. Set only what the host needs.
    void setOnVirtualKeyPressed(VirtualKeyCallback callback);
    void setOnVirtualKeyReleased(VirtualKeyCallback callback);
    void setOnSpeedChanged(SpeedCallback callback);
    void setOnPaletteChanged(PaletteCallback callback);
    void setOnComputerThemeChanged(ComputerThemeCallback callback);
    void setOnSaveStateRequested(ActionCallback callback);
    void setOnLoadStateRequested(ActionCallback callback);
    void setOnWaveformChanged(WaveformCallback callback);

    // Compatibility convenience for hosts that only need CHIP-8 key values.
    // The callback receives 0x0..0xF; non-keypad virtual controls are not sent
    // through this legacy channel.
    using LegacyKeyCallback = void (*)(int chip8HexKey, bool pressed, void* userdata);
    void setKeyCallback(LegacyKeyCallback callback, void* userdata);

    // Pixel text helpers are also exposed as free functions below.
    static constexpr int speedValueCount() { return 9; }
    static const int* speedValues();

private:
    struct Layout {
        SDL_Rect body;
        int depthX;
        int depthY;
        SDL_Rect bezel;
        SDL_Rect glass;
        SDL_Rect screen;
        SDL_Point deck[4];
        int deckFaceH;
        std::array<SDL_Rect, virtualKeyCount()> virtualKeys;
        int keyDepth;
        SDL_Rect brandPlate;
        SDL_Rect cartSlot;
        SDL_Rect powerLed;
        SDL_Point speedKnobCenter;
        int speedKnobRadius;
        SDL_Point paletteKnobCenter;
        int paletteKnobRadius;
        SDL_Rect waveformBezel;
        SDL_Rect waveformScreen;
        SDL_Point soundKnobCenter;
        int soundKnobRadius;
        SDL_Rect soundLed;
    };

    void computeLayout(int w, int h);

    void drawBackground();
    void drawGroundShadow();
    void drawComputerBody();
    void drawBezelAndGlass();
    void drawCrtEffects();
    void drawDecorations();
    void drawUpperControls();
    void drawWaveformPanel();
    void drawSaveLoad();
    void drawKeyboard();
    void drawVirtualKey(VirtualKey key);
    void drawKnob(SDL_Point center, int radius, int index, int count,
                  const char* title, const char* value, bool valueBelow);
    void drawHardwareDetails();

    void setColor(Color c) const;
    void setColor(Color c, uint8_t alphaOverride) const;
    void fillQuad(int x0, int y0, int x1, int y1,
                  int x2, int y2, int x3, int y3) const;
    void fillPoly(const SDL_Point* points, int count) const;
    void thickLine(int x0, int y0, int x1, int y1, int thickness) const;
    void drawFilledCircle(int cx, int cy, int radius) const;
    void drawCircleTicks(SDL_Point center, int radius, int count, int selected) const;
    static SDL_Rect inflate(SDL_Rect r, int d);
    void drawText(const char* s, int x, int y, int scale, Color c) const;
    void drawTextCentered(const char* s, int centerX, int y, int scale, Color c) const;
    void drawArrowGlyph(SDL_Rect rect, VirtualKey key, Color c) const;

    int virtualKeyAtPoint(int x, int y) const;
    bool pointInCircle(int x, int y, SDL_Point center, int radius) const;
    void pressVirtualKey(int index);
    void releaseVirtualKey(int index);
    void emitVirtualKey(VirtualKey key, bool pressed);
    bool isKeyVisuallyActive(int index) const;

    SDL_Renderer* ren_;
    int winW_;
    int winH_;
    float scale_;
    int originX_;
    int originY_;

    Layout lay_;
    ComputerTheme computerTheme_;
    ComputerThemeDef theme_;
    PaletteID displayPalette_;
    DisplayPaletteDef display_;
    Waveform waveform_;
    int speedIndex_;
    bool soundActive_;

    std::array<bool, virtualKeyCount()> keyPressed_;
    std::array<uint32_t, virtualKeyCount()> keyFlashStart_;
    int mouseKey_;

    VirtualKeyCallback onVirtualKeyPressed_;
    VirtualKeyCallback onVirtualKeyReleased_;
    SpeedCallback onSpeedChanged_;
    PaletteCallback onPaletteChanged_;
    ComputerThemeCallback onComputerThemeChanged_;
    ActionCallback onSaveStateRequested_;
    ActionCallback onLoadStateRequested_;
    WaveformCallback onWaveformChanged_;
    LegacyKeyCallback legacyKeyCallback_;
    void* legacyKeyCallbackUserdata_;
};

// Shared 3x5 pixel font used by the GUI and demo. Uppercase A-Z, 0-9 and a
// few symbols are supported; unknown glyphs are intentionally blank.
void drawPixelText(SDL_Renderer* renderer, const char* text, int x, int y, int scale);
int pixelTextWidth(const char* text, int scale);

} // namespace retro_gui

// Compatibility aliases for code that used the earlier GUI package draft.
using RCPalette = retro_gui::PaletteID;
using RCColor = retro_gui::Color;
using RetroComputerUI = retro_gui::RetroComputerUI;

#endif // RETRO_GUI_RETRO_COMPUTER_UI_H
