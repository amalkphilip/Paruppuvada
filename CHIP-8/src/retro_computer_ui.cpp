#include "retro_computer_ui.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace retro_gui {
namespace {

constexpr int DESIGN_W = 1280;
constexpr int DESIGN_H = 800;
constexpr float PI = 3.14159265358979323846f;

// Draw order is the visible keypad order, not numeric CHIP-8 order.
constexpr VirtualKey HEX_KEYPAD[16] = {
    VirtualKey::KEY_1, VirtualKey::KEY_2, VirtualKey::KEY_3, VirtualKey::KEY_C,
    VirtualKey::KEY_4, VirtualKey::KEY_5, VirtualKey::KEY_6, VirtualKey::KEY_D,
    VirtualKey::KEY_7, VirtualKey::KEY_8, VirtualKey::KEY_9, VirtualKey::KEY_E,
    VirtualKey::KEY_A, VirtualKey::KEY_0, VirtualKey::KEY_B, VirtualKey::KEY_F
};

int keyIndex(VirtualKey key) {
    return static_cast<int>(key);
}

VirtualKey keyFromIndex(int index) {
    if (index < 0 || index >= virtualKeyCount()) return VirtualKey::COUNT;
    return static_cast<VirtualKey>(index);
}

uint16_t glyphFor(char c) {
    switch (c) {
        case 'A': return 075755; case 'B': return 065656; case 'C': return 074447;
        case 'D': return 065556; case 'E': return 074747; case 'F': return 074744;
        case 'G': return 074757; case 'H': return 055755; case 'I': return 072227;
        case 'J': return 011157; case 'K': return 055655; case 'L': return 044447;
        case 'M': return 057555; case 'N': return 065555; case 'O': return 075557;
        case 'P': return 075744; case 'Q': return 075571; case 'R': return 075655;
        case 'S': return 074717; case 'T': return 072222; case 'U': return 055557;
        case 'V': return 055552; case 'W': return 055575; case 'X': return 055255;
        case 'Y': return 055222; case 'Z': return 071247;
        case '0': return 075557; case '1': return 026227; case '2': return 071747;
        case '3': return 071717; case '4': return 055711; case '5': return 074717;
        case '6': return 074757; case '7': return 071111; case '8': return 075757;
        case '9': return 075717;
        case '>': return 042124; case '<': return 012421; case ':': return 002020;
        case '-': return 000700; case '+': return 022722; case '[': return 064446;
        case ']': return 031113; case '.': return 000004; case ' ': return 000000;
        default: return 000000;
    }
}

} // namespace

void drawPixelText(SDL_Renderer* renderer, const char* text, int x, int y, int scale) {
    if (!renderer || !text || scale < 1) return;
    for (const char* p = text; *p; ++p) {
        const uint16_t bitmap = glyphFor(*p);
        for (int row = 0; row < 5; ++row) {
            const int bits = (bitmap >> (12 - row * 3)) & 7;
            for (int col = 0; col < 3; ++col) {
                if (bits & (4 >> col)) {
                    SDL_Rect px{x + col * scale, y + row * scale, scale, scale};
                    SDL_RenderFillRect(renderer, &px);
                }
            }
        }
        x += 4 * scale;
    }
}

int pixelTextWidth(const char* text, int scale) {
    if (!text || scale < 1) return 0;
    return static_cast<int>(std::strlen(text)) * 4 * scale;
}

RetroComputerUI::RetroComputerUI()
    : ren_(nullptr),
      winW_(DESIGN_W),
      winH_(DESIGN_H),
      scale_(1.0f),
      originX_(0),
      originY_(0),
      computerTheme_(ComputerTheme::WHITE),
      theme_(builtinComputerTheme(computerTheme_)),
      displayPalette_(PaletteID::CLASSIC_GREEN),
      display_(builtinDisplayPalette(displayPalette_)),
      waveform_(Waveform::SINE),
      speedIndex_(6),
      soundActive_(true),
      mouseKey_(-1),
      legacyKeyCallback_(nullptr),
      legacyKeyCallbackUserdata_(nullptr) {
    keyPressed_.fill(false);
    keyFlashStart_.fill(0);
    std::memset(&lay_, 0, sizeof(lay_));
}

RetroComputerUI::~RetroComputerUI() {
    shutdown();
}

bool RetroComputerUI::init(SDL_Renderer* renderer, int windowW, int windowH) {
    if (!renderer) return false;
    ren_ = renderer;
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    SDL_SetRenderDrawBlendMode(ren_, SDL_BLENDMODE_BLEND);
    computeLayout(windowW, windowH);
    return true;
}

void RetroComputerUI::shutdown() {
    ren_ = nullptr;
}

const int* RetroComputerUI::speedValues() {
    static const int values[speedValueCount()] = {1, 2, 4, 6, 8, 10, 12, 16, 20};
    return values;
}

void RetroComputerUI::computeLayout(int w, int h) {
    winW_ = w;
    winH_ = h;
    scale_ = std::min(static_cast<float>(w) / DESIGN_W,
                      static_cast<float>(h) / DESIGN_H);
    originX_ = static_cast<int>((w - DESIGN_W * scale_) * 0.5f);
    originY_ = static_cast<int>((h - DESIGN_H * scale_) * 0.5f);

    auto R = [&](int x, int y, int rw, int rh) {
        SDL_Rect r{originX_ + static_cast<int>(x * scale_),
                   originY_ + static_cast<int>(y * scale_),
                   std::max(1, static_cast<int>(rw * scale_)),
                   std::max(1, static_cast<int>(rh * scale_))};
        return r;
    };
    auto P = [&](int x, int y) {
        return SDL_Point{originX_ + static_cast<int>(x * scale_),
                         originY_ + static_cast<int>(y * scale_)};
    };
    auto S = [&](int v) { return std::max(1, static_cast<int>(v * scale_)); };

    lay_.body = R(50, 60, 1000, 440);
    lay_.depthX = S(44);
    lay_.depthY = -S(32);
    lay_.screen = R(100, 120, 512, 256);
    lay_.bezel = inflate(lay_.screen, S(22));
    lay_.glass = inflate(lay_.screen, S(6));

    lay_.deck[0] = P(50, 500);
    lay_.deck[1] = P(1050, 500);
    lay_.deck[2] = P(1080, 780);
    lay_.deck[3] = P(20, 780);
    lay_.deckFaceH = S(28);
    lay_.keyDepth = S(6);

    lay_.speedKnobCenter = P(700, 300);
    lay_.speedKnobRadius = S(34);
    lay_.paletteKnobCenter = P(820, 300);
    lay_.paletteKnobRadius = S(34);

    lay_.waveformBezel = R(650, 120, 350, 100);
    lay_.waveformScreen = R(670, 136, 310, 68);
    lay_.soundKnobCenter = P(940, 300);
    lay_.soundKnobRadius = S(31);
    lay_.soundLed = R(933, 375, 14, 14);

    lay_.virtualKeys[keyIndex(VirtualKey::SAVE)] = R(150, 540, 120, 40);
    lay_.virtualKeys[keyIndex(VirtualKey::LOAD)] = R(290, 540, 120, 40);

    int y = 560;
    for (int row = 0; row < 4; ++row) {
        const int kw = 60 + row * 2;
        const int kh = 38;
        const int gap = 12;
        const int rowW = 4 * kw + 3 * gap;
        int x = 800 - rowW / 2;
        for (int col = 0; col < 4; ++col) {
            const VirtualKey key = HEX_KEYPAD[row * 4 + col];
            lay_.virtualKeys[keyIndex(key)] = R(x, y, kw, kh);
            x += kw + gap;
        }
        y += kh + 10;
    }

    lay_.virtualKeys[keyIndex(VirtualKey::ARROW_UP)] = R(220, 610, 60, 35);
    lay_.virtualKeys[keyIndex(VirtualKey::ARROW_LEFT)] = R(150, 655, 60, 35);
    lay_.virtualKeys[keyIndex(VirtualKey::ARROW_DOWN)] = R(220, 655, 60, 35);
    lay_.virtualKeys[keyIndex(VirtualKey::ARROW_RIGHT)] = R(290, 655, 60, 35);
    lay_.virtualKeys[keyIndex(VirtualKey::SPACE)] = R(650, 755, 300, 40);

    lay_.brandPlate = R(780, 415, 230, 45);
    lay_.cartSlot = R(560, 430, 200, 14);
    lay_.powerLed = R(1025, 427, 20, 20);
}

void RetroComputerUI::onWindowResize(int w, int h) {
    computeLayout(w, h);
}

SDL_Rect RetroComputerUI::getCRTContentRect() const {
    return lay_.screen;
}

void RetroComputerUI::setContentRect(SDL_Rect rect) {
    lay_.screen = rect;
    lay_.bezel = inflate(rect, std::max(1, static_cast<int>(22 * scale_)));
    lay_.glass = inflate(rect, std::max(1, static_cast<int>(6 * scale_)));
}

void RetroComputerUI::setVirtualKeyPressed(VirtualKey key, bool pressed) {
    const int index = keyIndex(key);
    if (index < 0 || index >= virtualKeyCount()) return;
    if (pressed && !keyPressed_[index]) keyFlashStart_[index] = SDL_GetTicks();
    keyPressed_[index] = pressed;
}

void RetroComputerUI::setVirtualKeyPressed(int chip8HexKey, bool pressed) {
    if (chip8HexKey < 0 || chip8HexKey > 0xF) return;
    const char hexLabel = "0123456789ABCDEF"[chip8HexKey];
    for (int i = 0; i < 16; ++i) {
        if (virtualKeyName(HEX_KEYPAD[i])[0] == hexLabel) {
            setVirtualKeyPressed(HEX_KEYPAD[i], pressed);
            return;
        }
    }
}

bool RetroComputerUI::isVirtualKeyPressed(VirtualKey key) const {
    const int index = keyIndex(key);
    return index >= 0 && index < virtualKeyCount() && keyPressed_[index];
}

bool RetroComputerUI::isVirtualKeyPressed(int chip8HexKey) const {
    if (chip8HexKey < 0 || chip8HexKey > 0xF) return false;
    const char hexLabel = "0123456789ABCDEF"[chip8HexKey];
    for (int i = 0; i < 16; ++i) {
        if (virtualKeyName(HEX_KEYPAD[i])[0] == hexLabel) {
            return isVirtualKeyPressed(HEX_KEYPAD[i]);
        }
    }
    return false;
}

void RetroComputerUI::setComputerTheme(ComputerTheme theme) {
    if (theme == computerTheme_) return;
    computerTheme_ = theme;
    theme_ = builtinComputerTheme(theme);
    if (onComputerThemeChanged_) onComputerThemeChanged_(theme);
}

ComputerTheme RetroComputerUI::getComputerTheme() const {
    return computerTheme_;
}

void RetroComputerUI::setDisplayPalette(PaletteID palette) {
    if (palette == displayPalette_) return;
    displayPalette_ = palette;
    display_ = builtinDisplayPalette(palette);
    if (onPaletteChanged_) onPaletteChanged_(palette);
}

PaletteID RetroComputerUI::getDisplayPalette() const {
    return displayPalette_;
}

void RetroComputerUI::setWaveform(Waveform waveform) {
    if (waveform == waveform_) return;
    waveform_ = waveform;
    if (onWaveformChanged_) onWaveformChanged_(waveform_);
}

Waveform RetroComputerUI::getWaveform() const {
    return waveform_;
}

void RetroComputerUI::setSpeedIndex(int index) {
    index = std::max(0, std::min(index, speedValueCount() - 1));
    if (index == speedIndex_) return;
    speedIndex_ = index;
    if (onSpeedChanged_) onSpeedChanged_(getSpeedValue());
}

int RetroComputerUI::getSpeedIndex() const {
    return speedIndex_;
}

int RetroComputerUI::getSpeedValue() const {
    return speedValues()[speedIndex_];
}

void RetroComputerUI::setSoundActive(bool active) {
    soundActive_ = active;
}

bool RetroComputerUI::isSoundActive() const {
    return soundActive_;
}

void RetroComputerUI::setOnVirtualKeyPressed(VirtualKeyCallback callback) {
    onVirtualKeyPressed_ = std::move(callback);
}

void RetroComputerUI::setOnVirtualKeyReleased(VirtualKeyCallback callback) {
    onVirtualKeyReleased_ = std::move(callback);
}

void RetroComputerUI::setOnSpeedChanged(SpeedCallback callback) {
    onSpeedChanged_ = std::move(callback);
}

void RetroComputerUI::setOnPaletteChanged(PaletteCallback callback) {
    onPaletteChanged_ = std::move(callback);
}

void RetroComputerUI::setOnComputerThemeChanged(ComputerThemeCallback callback) {
    onComputerThemeChanged_ = std::move(callback);
}

void RetroComputerUI::setOnSaveStateRequested(ActionCallback callback) {
    onSaveStateRequested_ = std::move(callback);
}

void RetroComputerUI::setOnLoadStateRequested(ActionCallback callback) {
    onLoadStateRequested_ = std::move(callback);
}

void RetroComputerUI::setOnWaveformChanged(WaveformCallback callback) {
    onWaveformChanged_ = std::move(callback);
}

void RetroComputerUI::setKeyCallback(LegacyKeyCallback callback, void* userdata) {
    legacyKeyCallback_ = callback;
    legacyKeyCallbackUserdata_ = userdata;
}

bool RetroComputerUI::pointInCircle(int x, int y, SDL_Point center, int radius) const {
    const int dx = x - center.x;
    const int dy = y - center.y;
    return dx * dx + dy * dy <= radius * radius;
}

int RetroComputerUI::virtualKeyAtPoint(int x, int y) const {
    const SDL_Point pt{x, y};
    for (int i = 0; i < virtualKeyCount(); ++i) {
        SDL_Rect r = lay_.virtualKeys[i];
        if (r.w <= 0 || r.h <= 0) continue;
        r.h += lay_.keyDepth;
        if (SDL_PointInRect(&pt, &r)) return i;
    }
    return -1;
}

void RetroComputerUI::emitVirtualKey(VirtualKey key, bool pressed) {
    if (pressed) {
        if (onVirtualKeyPressed_) onVirtualKeyPressed_(key, true);
        if (key == VirtualKey::SAVE && onSaveStateRequested_) onSaveStateRequested_();
        if (key == VirtualKey::LOAD && onLoadStateRequested_) onLoadStateRequested_();
    } else if (onVirtualKeyReleased_) {
        onVirtualKeyReleased_(key, false);
    }

    const int hex = virtualKeyToChip8Hex(key);
    if (hex >= 0 && legacyKeyCallback_) {
        legacyKeyCallback_(hex, pressed, legacyKeyCallbackUserdata_);
    }
}

void RetroComputerUI::pressVirtualKey(int index) {
    if (index < 0 || index >= virtualKeyCount()) return;
    if (!keyPressed_[index]) {
        keyPressed_[index] = true;
        keyFlashStart_[index] = SDL_GetTicks();
        emitVirtualKey(keyFromIndex(index), true);
    }
}

void RetroComputerUI::releaseVirtualKey(int index) {
    if (index < 0 || index >= virtualKeyCount()) return;
    if (keyPressed_[index]) {
        keyPressed_[index] = false;
        emitVirtualKey(keyFromIndex(index), false);
    }
}

void RetroComputerUI::handleEvent(const SDL_Event& e) {
    if (e.type == SDL_MOUSEBUTTONDOWN) {
        const int x = e.button.x;
        const int y = e.button.y;
        const bool forward = e.button.button != SDL_BUTTON_RIGHT;

        if (pointInCircle(x, y, lay_.speedKnobCenter, lay_.speedKnobRadius + 4)) {
            const int delta = forward ? 1 : -1;
            int next = (speedIndex_ + delta + speedValueCount()) % speedValueCount();
            setSpeedIndex(next);
            return;
        }
        if (pointInCircle(x, y, lay_.paletteKnobCenter, lay_.paletteKnobRadius + 4)) {
            setDisplayPalette(forward ? nextPalette(displayPalette_)
                                      : previousPalette(displayPalette_));
            return;
        }
        if (pointInCircle(x, y, lay_.soundKnobCenter, lay_.soundKnobRadius + 4)) {
            setWaveform(forward ? nextWaveform(waveform_) : previousWaveform(waveform_));
            return;
        }

        if (e.button.button == SDL_BUTTON_LEFT) {
            const int key = virtualKeyAtPoint(x, y);
            if (key >= 0) {
                mouseKey_ = key;
                pressVirtualKey(key);
            }
        }
    } else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
        if (mouseKey_ >= 0) {
            const int key = mouseKey_;
            mouseKey_ = -1;
            releaseVirtualKey(key);
        }
    } else if (e.type == SDL_WINDOWEVENT &&
               e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
        computeLayout(e.window.data1, e.window.data2);
    }
}

void RetroComputerUI::setColor(Color c) const {
    SDL_SetRenderDrawColor(ren_, c.r, c.g, c.b, c.a);
}

void RetroComputerUI::setColor(Color c, uint8_t a) const {
    SDL_SetRenderDrawColor(ren_, c.r, c.g, c.b, a);
}

SDL_Rect RetroComputerUI::inflate(SDL_Rect r, int d) {
    return SDL_Rect{r.x - d, r.y - d, r.w + 2 * d, r.h + 2 * d};
}

void RetroComputerUI::fillPoly(const SDL_Point* pts, int n) const {
    if (!pts || n < 3) return;
    int minY = pts[0].y;
    int maxY = pts[0].y;
    for (int i = 1; i < n; ++i) {
        minY = std::min(minY, pts[i].y);
        maxY = std::max(maxY, pts[i].y);
    }
    for (int y = minY; y <= maxY; ++y) {
        float xs[8];
        int count = 0;
        for (int i = 0; i < n && count < 8; ++i) {
            const SDL_Point a = pts[i];
            const SDL_Point b = pts[(i + 1) % n];
            if ((a.y <= y && b.y > y) || (b.y <= y && a.y > y)) {
                const float t = static_cast<float>(y - a.y) /
                                static_cast<float>(b.y - a.y);
                xs[count++] = a.x + t * (b.x - a.x);
            }
        }
        std::sort(xs, xs + count);
        for (int i = 0; i + 1 < count; i += 2) {
            SDL_RenderDrawLine(ren_, static_cast<int>(xs[i]), y,
                               static_cast<int>(xs[i + 1]), y);
        }
    }
}

void RetroComputerUI::fillQuad(int x0, int y0, int x1, int y1,
                               int x2, int y2, int x3, int y3) const {
    const SDL_Point pts[4] = {{x0, y0}, {x1, y1}, {x2, y2}, {x3, y3}};
    fillPoly(pts, 4);
}

void RetroComputerUI::thickLine(int x0, int y0, int x1, int y1, int thickness) const {
    thickness = std::max(1, thickness);
    for (int i = 0; i < thickness; ++i) {
        if (y0 == y1) SDL_RenderDrawLine(ren_, x0, y0 + i, x1, y1 + i);
        else if (x0 == x1) SDL_RenderDrawLine(ren_, x0 + i, y0, x1 + i, y1);
        else SDL_RenderDrawLine(ren_, x0, y0 + i, x1, y1 + i);
    }
}

void RetroComputerUI::drawFilledCircle(int cx, int cy, int radius) const {
    for (int y = -radius; y <= radius; ++y) {
        const int half = static_cast<int>(std::sqrt(
            static_cast<float>(radius * radius - y * y)));
        SDL_RenderDrawLine(ren_, cx - half, cy + y, cx + half, cy + y);
    }
}

void RetroComputerUI::drawCircleTicks(SDL_Point center, int radius, int count,
                                      int selected) const {
    if (count <= 1) return;
    for (int i = 0; i < count; ++i) {
        const float angle = (-135.0f + 270.0f * i / (count - 1)) * PI / 180.0f;
        const int x0 = center.x + static_cast<int>(std::cos(angle) * (radius + 7 * scale_));
        const int y0 = center.y + static_cast<int>(std::sin(angle) * (radius + 7 * scale_));
        const int x1 = center.x + static_cast<int>(std::cos(angle) * (radius + 12 * scale_));
        const int y1 = center.y + static_cast<int>(std::sin(angle) * (radius + 12 * scale_));
        setColor(i == selected ? theme_.accent : theme_.bodyEdge,
                 i == selected ? 255 : 150);
        SDL_RenderDrawLine(ren_, x0, y0, x1, y1);
    }
}

void RetroComputerUI::drawText(const char* s, int x, int y, int scale, Color c) const {
    setColor(c);
    drawPixelText(ren_, s, x, y, scale);
}

void RetroComputerUI::drawTextCentered(const char* s, int centerX, int y,
                                       int scale, Color c) const {
    drawText(s, centerX - pixelTextWidth(s, scale) / 2, y, scale, c);
}

void RetroComputerUI::drawBackground() {
    setColor(theme_.background);
    SDL_RenderClear(ren_);

    setColor(theme_.backgroundDots);
    const int step = std::max(6, static_cast<int>(14 * scale_));
    const int dot = std::max(1, static_cast<int>(2 * scale_));
    for (int y = step / 2; y < winH_; y += step) {
        for (int x = step / 2; x < winW_; x += step) {
            SDL_Rect d{x, y, dot, dot};
            SDL_RenderFillRect(ren_, &d);
        }
    }
}

void RetroComputerUI::drawGroundShadow() {
    const int frontY = lay_.deck[3].y + lay_.deckFaceH;
    const int left = lay_.deck[3].x - static_cast<int>(30 * scale_);
    const int right = lay_.deck[2].x + static_cast<int>(60 * scale_);
    SDL_Rect base{left, frontY - static_cast<int>(4 * scale_),
                  right - left, static_cast<int>(34 * scale_)};
    const uint8_t alpha[3] = {90, 50, 25};
    for (int i = 0; i < 3; ++i) {
        SDL_Rect r = inflate(base, i * static_cast<int>(8 * scale_));
        r.y -= i * static_cast<int>(2 * scale_);
        setColor(theme_.bezelDark, alpha[i]);
        SDL_RenderFillRect(ren_, &r);
    }
}

void RetroComputerUI::drawComputerBody() {
    const SDL_Rect& b = lay_.body;
    const int dx = lay_.depthX;
    const int dy = lay_.depthY;

    setColor(theme_.bodyLight);
    fillQuad(b.x, b.y, b.x + b.w, b.y,
             b.x + b.w + dx, b.y + dy, b.x + dx, b.y + dy);
    setColor(theme_.bodyEdge);
    thickLine(b.x + dx, b.y + dy, b.x + b.w + dx, b.y + dy,
              std::max(1, static_cast<int>(2 * scale_)));

    setColor(theme_.bodyDark);
    fillQuad(b.x + b.w, b.y, b.x + b.w + dx, b.y + dy,
             b.x + b.w + dx, b.y + b.h + dy, b.x + b.w, b.y + b.h);
    setColor(theme_.bezelDark, 60);
    fillQuad(b.x + b.w + dx / 2, b.y + dy / 2,
             b.x + b.w + dx, b.y + dy,
             b.x + b.w + dx, b.y + b.h + dy,
             b.x + b.w + dx / 2, b.y + b.h + dy / 2);

    // Side vents preserve the approved hardware detail.
    setColor(theme_.bezelDark);
    for (int i = 0; i < 5; ++i) {
        const int vy = b.y + static_cast<int>((60 + i * 52) * scale_);
        fillQuad(b.x + b.w + static_cast<int>(8 * scale_), vy,
                 b.x + b.w + dx - static_cast<int>(8 * scale_), vy + dy / 2,
                 b.x + b.w + dx - static_cast<int>(8 * scale_), vy + dy / 2 + static_cast<int>(6 * scale_),
                 b.x + b.w + static_cast<int>(8 * scale_), vy + static_cast<int>(6 * scale_));
    }

    setColor(theme_.bodyMid);
    SDL_RenderFillRect(ren_, &b);
    setColor(theme_.bodyEdge);
    thickLine(b.x, b.y, b.x + b.w, b.y, std::max(1, static_cast<int>(3 * scale_)));
    thickLine(b.x, b.y, b.x, b.y + b.h, std::max(1, static_cast<int>(2 * scale_)));
    setColor(theme_.bodyDark);
    thickLine(b.x, b.y + b.h - std::max(1, static_cast<int>(3 * scale_)), b.x + b.w,
              b.y + b.h - std::max(1, static_cast<int>(3 * scale_)),
              std::max(1, static_cast<int>(3 * scale_)));

    const SDL_Point* d = lay_.deck;
    setColor(theme_.bodyDark);
    fillQuad(d[1].x, d[1].y, d[2].x, d[2].y,
             d[2].x, d[2].y + lay_.deckFaceH,
             d[1].x, d[1].y + lay_.deckFaceH / 2);
    fillQuad(d[3].x, d[3].y, d[2].x, d[2].y,
             d[2].x, d[2].y + lay_.deckFaceH,
             d[3].x, d[3].y + lay_.deckFaceH);

    setColor(theme_.bodyMid);
    fillQuad(d[0].x, d[0].y, d[1].x, d[1].y, d[2].x, d[2].y, d[3].x, d[3].y);

    // Panel shading and physical seams.
    setColor(theme_.bezelDark, 22);
    fillQuad(d[3].x, d[3].y - static_cast<int>(44 * scale_),
             d[2].x, d[2].y - static_cast<int>(44 * scale_),
             d[2].x, d[2].y, d[3].x, d[3].y);
    setColor(theme_.bezelDark, 110);
    thickLine(d[0].x, d[0].y + 1, d[1].x, d[1].y + 1,
              std::max(1, static_cast<int>(2 * scale_)));
    setColor(theme_.bodyEdge);
    thickLine(d[0].x, d[0].y, d[1].x, d[1].y,
              std::max(1, static_cast<int>(2 * scale_)));
    setColor(theme_.bodyLight);
    thickLine(d[3].x, d[3].y, d[2].x, d[2].y,
              std::max(1, static_cast<int>(2 * scale_)));

    const int s = std::max(2, static_cast<int>(4 * scale_));
    const int m = static_cast<int>(12 * scale_);
    const SDL_Point screws[4] = {
        {b.x + m, b.y + m}, {b.x + b.w - m, b.y + m},
        {b.x + m, b.y + b.h - m}, {b.x + b.w - m, b.y + b.h - m}
    };
    for (const SDL_Point& screw : screws) {
        setColor(theme_.bodyDark);
        SDL_Rect sq{screw.x - s / 2, screw.y - s / 2, s, s};
        SDL_RenderFillRect(ren_, &sq);
        setColor(theme_.bodyLight);
        SDL_RenderDrawLine(ren_, sq.x, sq.y, sq.x + sq.w - 1, sq.y);
    }
}

void RetroComputerUI::drawBezelAndGlass() {
    const SDL_Rect& bz = lay_.bezel;
    const SDL_Rect& gl = lay_.glass;

    setColor(theme_.bezel);
    SDL_RenderFillRect(ren_, &bz);
    setColor(theme_.bodyLight);
    thickLine(bz.x, bz.y, bz.x + bz.w, bz.y, std::max(1, static_cast<int>(3 * scale_)));
    thickLine(bz.x, bz.y, bz.x, bz.y + bz.h, std::max(1, static_cast<int>(3 * scale_)));
    setColor(theme_.bezelDark);
    thickLine(bz.x, bz.y + bz.h - 2, bz.x + bz.w, bz.y + bz.h - 2,
              std::max(1, static_cast<int>(3 * scale_)));
    thickLine(bz.x + bz.w - 2, bz.y, bz.x + bz.w - 2, bz.y + bz.h,
              std::max(1, static_cast<int>(3 * scale_)));

    setColor(theme_.bezelDark);
    thickLine(gl.x - 3, gl.y - 3, gl.x + gl.w + 3, gl.y - 3,
              std::max(1, static_cast<int>(4 * scale_)));
    thickLine(gl.x - 3, gl.y - 3, gl.x - 3, gl.y + gl.h + 3,
              std::max(1, static_cast<int>(4 * scale_)));
    setColor(theme_.bodyLight, 120);
    thickLine(gl.x - 1, gl.y + gl.h + 1, gl.x + gl.w + 1, gl.y + gl.h + 1,
              std::max(1, static_cast<int>(2 * scale_)));
    thickLine(gl.x + gl.w + 1, gl.y - 1, gl.x + gl.w + 1, gl.y + gl.h + 1,
              std::max(1, static_cast<int>(2 * scale_)));

    setColor(display_.glass);
    SDL_RenderFillRect(ren_, &gl);
}

void RetroComputerUI::renderContent(SDL_Texture* content) {
    setColor(display_.glow, 22);
    SDL_RenderFillRect(ren_, &lay_.glass);
    if (content) SDL_RenderCopy(ren_, content, nullptr, &lay_.screen);
}

void RetroComputerUI::drawCrtEffects() {
    const SDL_Rect& sc = lay_.screen;
    SDL_Rect clip = lay_.glass;
    SDL_RenderSetClipRect(ren_, &clip);

    const int lineStep = std::max(2, static_cast<int>(2 * scale_));
    const int lineH = std::max(1, static_cast<int>(1 * scale_));
    setColor(theme_.bezelDark, 46);
    for (int y = sc.y; y < sc.y + sc.h; y += lineStep * 2) {
        SDL_Rect line{sc.x, y, sc.w, lineH};
        SDL_RenderFillRect(ren_, &line);
    }

    const uint8_t flicker = static_cast<uint8_t>(6 + ((SDL_GetTicks() / 90) % 2) * 4);
    setColor(display_.glow, flicker);
    SDL_RenderFillRect(ren_, &sc);

    setColor(display_.foreground, 14);
    fillQuad(sc.x + static_cast<int>(14 * scale_), sc.y,
             sc.x + sc.w / 3, sc.y,
             sc.x + sc.w / 6, sc.y + sc.h,
             sc.x - static_cast<int>(30 * scale_), sc.y + sc.h);

    const int v = std::max(2, static_cast<int>(6 * scale_));
    setColor(theme_.bezelDark, 45);
    for (int i = 0; i < 3; ++i) {
        const int o = i * v / 2;
        const int t = v - i * 2;
        if (t < 1) break;
        SDL_Rect tl{sc.x + o, sc.y + o, v * 2, t};
        SDL_Rect tr{sc.x + sc.w - o - v * 2, sc.y + o, v * 2, t};
        SDL_Rect bl{sc.x + o, sc.y + sc.h - o - t, v * 2, t};
        SDL_Rect br{sc.x + sc.w - o - v * 2, sc.y + sc.h - o - t, v * 2, t};
        SDL_RenderFillRect(ren_, &tl);
        SDL_RenderFillRect(ren_, &tr);
        SDL_RenderFillRect(ren_, &bl);
        SDL_RenderFillRect(ren_, &br);
    }

    setColor(theme_.bezelDark);
    SDL_Rect inner = lay_.glass;
    for (int i = 0; i < std::max(1, static_cast<int>(3 * scale_)); ++i) {
        SDL_RenderDrawRect(ren_, &inner);
        inner = inflate(inner, -1);
    }
    SDL_RenderSetClipRect(ren_, nullptr);
}

void RetroComputerUI::drawDecorations() {
    const SDL_Rect& bp = lay_.brandPlate;
    setColor(theme_.bodyDark);
    SDL_RenderFillRect(ren_, &bp);
    setColor(theme_.bodyEdge, 140);
    SDL_RenderDrawRect(ren_, &bp);
    const char* brand = "PARUPPUVADA PC-8";
    const int ts = std::max(1, static_cast<int>(3 * scale_));
    drawTextCentered(brand, bp.x + bp.w / 2,
                     bp.y + (bp.h - 5 * ts) / 2, ts, theme_.textMain);

    const SDL_Rect& cs = lay_.cartSlot;
    setColor(theme_.bezelDark);
    SDL_RenderFillRect(ren_, &cs);
    setColor(theme_.bodyLight, 160);
    thickLine(cs.x, cs.y + cs.h, cs.x + cs.w, cs.y + cs.h,
              std::max(1, static_cast<int>(1 * scale_)));

    const SDL_Rect& led = lay_.powerLed;
    setColor(theme_.accent, 36);
    SDL_Rect glow = inflate(led, static_cast<int>(5 * scale_));
    SDL_RenderFillRect(ren_, &glow);
    setColor(theme_.accent);
    SDL_RenderFillRect(ren_, &led);
    setColor(theme_.bodyLight, 200);
    thickLine(led.x, led.y, led.x + led.w, led.y,
              std::max(1, static_cast<int>(2 * scale_)));
}

void RetroComputerUI::drawHardwareDetails() {
    // Small control-panel seams, screws and LEDs make the long upper panel feel
    // like mounted hardware rather than unused space.
    const int y1 = originY_ + static_cast<int>(686 * scale_);
    const int y2 = originY_ + static_cast<int>(902 * scale_);
    const int left = originX_ + static_cast<int>(220 * scale_);
    const int right = originX_ + static_cast<int>(740 * scale_);
    setColor(theme_.bodyDark, 130);
    thickLine(left, y1, right, y1, std::max(1, static_cast<int>(2 * scale_)));
    thickLine(left, y2, right, y2, std::max(1, static_cast<int>(2 * scale_)));
    setColor(theme_.bodyLight, 150);
    SDL_RenderDrawLine(ren_, left, y1 + 2, right, y1 + 2);
    SDL_RenderDrawLine(ren_, left, y2 + 2, right, y2 + 2);

    const int s = std::max(2, static_cast<int>(4 * scale_));
    const SDL_Point screws[4] = {
        {originX_ + static_cast<int>(224 * scale_), originY_ + static_cast<int>(574 * scale_)},
        {originX_ + static_cast<int>(736 * scale_), originY_ + static_cast<int>(574 * scale_)},
        {originX_ + static_cast<int>(224 * scale_), originY_ + static_cast<int>(902 * scale_)},
        {originX_ + static_cast<int>(736 * scale_), originY_ + static_cast<int>(902 * scale_)}
    };
    for (const SDL_Point& p : screws) {
        setColor(theme_.bodyEdge);
        SDL_Rect sq{p.x - s / 2, p.y - s / 2, s, s};
        SDL_RenderFillRect(ren_, &sq);
        setColor(theme_.bodyLight);
        SDL_RenderDrawLine(ren_, sq.x, sq.y, sq.x + sq.w - 1, sq.y);
    }

    // A few tiny ventilation slots beside the instrument panel.
    setColor(theme_.bodyDark, 170);
    for (int i = 0; i < 4; ++i) {
        SDL_Rect vent{originX_ + static_cast<int>((192 + i * 12) * scale_),
                      originY_ + static_cast<int>(736 * scale_),
                      std::max(2, static_cast<int>(6 * scale_)),
                      std::max(8, static_cast<int>(34 * scale_))};
        SDL_RenderFillRect(ren_, &vent);
        SDL_Rect ventR{originX_ + static_cast<int>((744 + i * 12) * scale_),
                       vent.y, vent.w, vent.h};
        SDL_RenderFillRect(ren_, &ventR);
    }
}

void RetroComputerUI::drawKnob(SDL_Point center, int radius, int index, int count,
                               const char* title, const char* value,
                               bool valueBelow) {
    const int sr = radius + std::max(2, static_cast<int>(4 * scale_));
    setColor(theme_.bodyDark);
    drawFilledCircle(center.x + static_cast<int>(3 * scale_),
                     center.y + static_cast<int>(4 * scale_), sr);
    setColor(theme_.bodyEdge);
    drawFilledCircle(center.x, center.y, sr);
    setColor(theme_.bodyLight);
    drawFilledCircle(center.x, center.y, radius);
    setColor(theme_.bodyMid);
    drawFilledCircle(center.x, center.y, std::max(1, radius - 6 * static_cast<int>(scale_)));
    setColor(theme_.bodyLight);
    drawFilledCircle(center.x - radius / 4, center.y - radius / 4,
                     std::max(2, radius / 4));

    drawCircleTicks(center, radius, count, index);

    const float angle = (-135.0f + 270.0f * index / std::max(1, count - 1)) * PI / 180.0f;
    const int px = center.x + static_cast<int>(std::cos(angle) * (radius - 8 * scale_));
    const int py = center.y + static_cast<int>(std::sin(angle) * (radius - 8 * scale_));
    setColor(theme_.textMain);
    thickLine(center.x, center.y, px, py, std::max(2, static_cast<int>(3 * scale_)));
    setColor(theme_.bodyDark);
    drawFilledCircle(center.x, center.y, std::max(3, static_cast<int>(6 * scale_)));

    const int titleScale = std::max(2, static_cast<int>(2.0f * scale_));
    const int valueScale = std::max(2, static_cast<int>(2.0f * scale_));
    if (valueBelow) {
        drawTextCentered(title, center.x,
                         center.y - radius - static_cast<int>(28 * scale_),
                         titleScale, theme_.textMain);
        drawTextCentered(value, center.x,
                         center.y + radius + static_cast<int>(14 * scale_),
                         valueScale, theme_.textMain);
    } else {
        drawTextCentered(title, center.x,
                         center.y - radius - static_cast<int>(28 * scale_),
                         titleScale, theme_.textMain);
    }
}

void RetroComputerUI::drawUpperControls() {
    char speedText[16];
    std::snprintf(speedText, sizeof(speedText), "%d CPS", getSpeedValue());
    drawKnob(lay_.speedKnobCenter, lay_.speedKnobRadius,
             speedIndex_, speedValueCount(), "CPU SPEED", speedText, true);
    drawKnob(lay_.paletteKnobCenter, lay_.paletteKnobRadius,
             static_cast<int>(displayPalette_), static_cast<int>(PaletteID::COUNT),
             "DISPLAY PALETTE", "", false);
             
    int yBase = lay_.paletteKnobCenter.y + lay_.paletteKnobRadius + static_cast<int>(12 * scale_);
    const int itemHeight = static_cast<int>(22 * scale_);
    const int ledSize = static_cast<int>(10 * scale_);
    const int textScale = std::max(1, static_cast<int>(2.0f * scale_));
    
    for (int i = 0; i < static_cast<int>(PaletteID::COUNT); ++i) {
        int y = yBase + i * itemHeight;
        int xLed = lay_.paletteKnobCenter.x - static_cast<int>(45 * scale_);
        int xText = xLed + ledSize + static_cast<int>(8 * scale_);
        
        bool isActive = (i == static_cast<int>(displayPalette_));
        
        SDL_Rect ledRect = {xLed, y, ledSize, ledSize};
        if (isActive) {
            setColor(theme_.accent, 255);
            SDL_RenderFillRect(ren_, &ledRect);
            SDL_Rect glow = inflate(ledRect, static_cast<int>(3 * scale_));
            setColor(theme_.accent, 80);
            SDL_RenderFillRect(ren_, &glow);
        } else {
            setColor(theme_.bodyDark);
            SDL_RenderFillRect(ren_, &ledRect);
        }
        
        drawText(paletteName(static_cast<PaletteID>(i)), xText, y + (ledSize - textScale * 5) / 2, textScale, isActive ? theme_.accent : theme_.textMain);
    }
}

void RetroComputerUI::drawWaveformPanel() {
    const SDL_Rect& bezel = lay_.waveformBezel;
    const SDL_Rect& screen = lay_.waveformScreen;

    // Recessed instrument bezel.
    setColor(theme_.bodyDark);
    SDL_Rect shadow = bezel;
    shadow.x += static_cast<int>(4 * scale_);
    shadow.y += static_cast<int>(5 * scale_);
    SDL_RenderFillRect(ren_, &shadow);
    setColor(theme_.bezel);
    SDL_RenderFillRect(ren_, &bezel);
    setColor(theme_.bodyLight);
    thickLine(bezel.x, bezel.y, bezel.x + bezel.w, bezel.y,
              std::max(1, static_cast<int>(3 * scale_)));
    thickLine(bezel.x, bezel.y, bezel.x, bezel.y + bezel.h,
              std::max(1, static_cast<int>(3 * scale_)));
    setColor(theme_.bezelDark);
    thickLine(bezel.x, bezel.y + bezel.h - 2, bezel.x + bezel.w, bezel.y + bezel.h - 2,
              std::max(1, static_cast<int>(3 * scale_)));
    thickLine(bezel.x + bezel.w - 2, bezel.y, bezel.x + bezel.w - 2, bezel.y + bezel.h,
              std::max(1, static_cast<int>(3 * scale_)));

    // Inner recess and dark glass.
    setColor(theme_.bezelDark);
    thickLine(screen.x - 4, screen.y - 4, screen.x + screen.w + 4, screen.y - 4,
              std::max(1, static_cast<int>(3 * scale_)));
    thickLine(screen.x - 4, screen.y - 4, screen.x - 4, screen.y + screen.h + 4,
              std::max(1, static_cast<int>(3 * scale_)));
    setColor(theme_.bodyLight, 130);
    thickLine(screen.x - 2, screen.y + screen.h + 2,
              screen.x + screen.w + 2, screen.y + screen.h + 2,
              std::max(1, static_cast<int>(2 * scale_)));
    setColor(rgb(0x10, 0x1A, 0x14));
    SDL_RenderFillRect(ren_, &screen);

    SDL_RenderSetClipRect(ren_, &screen);

    // Oscilloscope graticule.
    const uint8_t gridAlpha = soundActive_ ? 72 : 38;
    setColor(theme_.accent, gridAlpha);
    const int gridX = std::max(8, static_cast<int>(20 * scale_));
    const int gridY = std::max(6, static_cast<int>(12 * scale_));
    for (int x = screen.x + gridX; x < screen.x + screen.w; x += gridX) {
        SDL_RenderDrawLine(ren_, x, screen.y, x, screen.y + screen.h);
    }
    for (int y = screen.y + gridY; y < screen.y + screen.h; y += gridY) {
        SDL_RenderDrawLine(ren_, screen.x, y, screen.x + screen.w, y);
    }
    setColor(theme_.accent, soundActive_ ? 130 : 70);
    SDL_RenderDrawLine(ren_, screen.x, screen.y + screen.h / 2,
                       screen.x + screen.w, screen.y + screen.h / 2);

    // Mathematical animated waveform, quantized to a low-resolution scope.
    const float speed = soundActive_ ? 0.00023f : 0.00005f;
    const float phase = static_cast<float>(SDL_GetTicks()) * speed;
    const int step = std::max(2, static_cast<int>(3 * scale_));
    const int amplitude = std::max(8, screen.h / 3);
    const int centerY = screen.y + screen.h / 2;
    const uint8_t waveAlpha = soundActive_ ? 235 : 95;
    int previousY = centerY;
    for (int x = screen.x + 8, sample = 0; x < screen.x + screen.w - 8;
         x += step, ++sample) {
        const float p = static_cast<float>(sample) * step /
                        static_cast<float>(screen.w) * 2.0f + phase;
        int y = centerY - static_cast<int>(waveformSample(waveform_, p) * amplitude);
        y = (y / step) * step;
        if (x > screen.x + 8) {
            setColor(theme_.accent, waveAlpha);
            SDL_RenderDrawLine(ren_, x - step, previousY, x, previousY);
            SDL_RenderDrawLine(ren_, x, previousY, x, y);
        }
        setColor(theme_.bodyLight, waveAlpha);
        SDL_Rect pixel{x - step / 2, y - step / 2, step, step};
        SDL_RenderFillRect(ren_, &pixel);
        previousY = y;
    }

    const int labelScale = std::max(2, static_cast<int>(2.0f * scale_));
    drawTextCentered(waveformName(waveform_), screen.x + screen.w / 2,
                     screen.y + screen.h - 5 * labelScale - 4,
                     labelScale, theme_.bodyLight);
    SDL_RenderSetClipRect(ren_, nullptr);

    const int titleScale = std::max(2, static_cast<int>(2.0f * scale_));
    drawTextCentered("WAVEFORM", bezel.x + bezel.w / 2,
                     bezel.y + 8, titleScale, theme_.textMain);

    // SOUND ACTIVE indicator, host-controlled only.
    const uint32_t pulse = (SDL_GetTicks() / 180) % 2;
    const uint8_t ledAlpha = soundActive_ ? static_cast<uint8_t>(170 + pulse * 70) : 70;
    setColor(theme_.accent, ledAlpha / 3);
    SDL_Rect glow = inflate(lay_.soundLed, static_cast<int>(4 * scale_));
    SDL_RenderFillRect(ren_, &glow);
    setColor(theme_.accent, ledAlpha);
    SDL_RenderFillRect(ren_, &lay_.soundLed);
    setColor(theme_.bodyLight, 180);
    SDL_RenderDrawLine(ren_, lay_.soundLed.x, lay_.soundLed.y,
                       lay_.soundLed.x + lay_.soundLed.w - 1, lay_.soundLed.y);

    // Draw knob without text value (symbols replace it)
    drawKnob(lay_.soundKnobCenter, lay_.soundKnobRadius,
             static_cast<int>(waveform_), static_cast<int>(Waveform::COUNT),
             "SOUND", "", false);

    // Draw waveform symbols to the right of the sound knob
    // Layout: knob center at (940,300), radius 31 -> knob right edge ~971
    // Symbols are stacked vertically to the right of the knob
    {
        const int waveCount = static_cast<int>(Waveform::COUNT);
        const int symW = static_cast<int>(32 * scale_);  // symbol box width
        const int symH = static_cast<int>(14 * scale_);  // symbol box height
        const int symGap = static_cast<int>(6 * scale_); // gap between symbols
        const int totalH = waveCount * symH + (waveCount - 1) * symGap;
        const int xLeft = lay_.soundKnobCenter.x + lay_.soundKnobRadius + static_cast<int>(14 * scale_);
        const int yTop  = lay_.soundKnobCenter.y - totalH / 2;

        for (int wi = 0; wi < waveCount; ++wi) {
            bool active = (wi == static_cast<int>(waveform_));
            uint8_t bodyAlpha = active ? 220 : 32;
            uint8_t rimAlpha  = active ? 255 : 55;

            int cx = xLeft;
            int cy = yTop + wi * (symH + symGap) + symH / 2;
            int hw = symW;
            int hh = symH / 2 - 1;

            Waveform wtype = static_cast<Waveform>(wi);

            // ── SQUARE: solid filled rectangle ────────────────────────────
            if (wtype == Waveform::SQUARE) {
                SDL_Rect rect = {cx, cy - hh, hw, hh * 2};
                setColor(theme_.accent, bodyAlpha);
                SDL_RenderFillRect(ren_, &rect);
                setColor(theme_.accent, rimAlpha);
                SDL_RenderDrawRect(ren_, &rect);
            }

            // ── TRIANGLE: solid filled triangle (peak at top) ─────────────
            else if (wtype == Waveform::TRIANGLE) {
                SDL_Point tri[3] = {
                    {cx + hw / 2, cy - hh},   // apex
                    {cx,          cy + hh},    // bottom-left
                    {cx + hw,     cy + hh}     // bottom-right
                };
                setColor(theme_.accent, bodyAlpha);
                fillPoly(tri, 3);
                // Bright outline
                setColor(theme_.accent, rimAlpha);
                SDL_RenderDrawLine(ren_, tri[0].x, tri[0].y, tri[1].x, tri[1].y);
                SDL_RenderDrawLine(ren_, tri[1].x, tri[1].y, tri[2].x, tri[2].y);
                SDL_RenderDrawLine(ren_, tri[2].x, tri[2].y, tri[0].x, tri[0].y);
            }

            // ── SINE: filled band tracing the sine shape ───────────────────
            else if (wtype == Waveform::SINE) {
                const int segs    = 24;
                const int bHalf   = std::max(2, static_cast<int>(3 * scale_));
                for (int s = 0; s < segs; ++s) {
                    float t1 = static_cast<float>(s)     / segs;
                    float t2 = static_cast<float>(s + 1) / segs;
                    int x1  = cx + static_cast<int>(t1 * hw);
                    int x2  = cx + static_cast<int>(t2 * hw);
                    int my1 = cy + static_cast<int>(-std::sin(t1 * 2.0f * PI) * hh);
                    int my2 = cy + static_cast<int>(-std::sin(t2 * 2.0f * PI) * hh);
                    SDL_Point quad[4] = {
                        {x1, my1 - bHalf}, {x2, my2 - bHalf},
                        {x2, my2 + bHalf}, {x1, my1 + bHalf}
                    };
                    setColor(theme_.accent, bodyAlpha);
                    fillPoly(quad, 4);
                    setColor(theme_.accent, rimAlpha);
                    SDL_RenderDrawLine(ren_, x1, my1, x2, my2);
                }
            }

            // ── SAWTOOTH: filled saw shape ─────────────────────────────────
            else if (wtype == Waveform::SAWTOOTH) {
                int mid = cx + hw / 2;
                int bh  = std::max(2, static_cast<int>(3 * scale_));
                SDL_Point t1[4] = {{cx,  cy+hh-bh}, {mid, cy-hh-bh}, {mid, cy-hh+bh}, {cx,  cy+hh+bh}};
                SDL_Rect  drop  = {mid - bh, cy - hh, bh*2, hh*2};
                SDL_Point t2[4] = {{mid, cy+hh-bh}, {cx+hw, cy-hh-bh}, {cx+hw, cy-hh+bh}, {mid, cy+hh+bh}};
                setColor(theme_.accent, bodyAlpha);
                fillPoly(t1, 4);
                SDL_RenderFillRect(ren_, &drop);
                fillPoly(t2, 4);
                setColor(theme_.accent, rimAlpha);
                SDL_RenderDrawLine(ren_, cx, cy+hh, mid, cy-hh);
                SDL_RenderDrawLine(ren_, mid, cy+hh, cx+hw, cy-hh);
            }
        }
    }
}

bool RetroComputerUI::isKeyVisuallyActive(int index) const {
    if (index < 0 || index >= virtualKeyCount()) return false;
    if (keyPressed_[index]) return true;
    const uint32_t elapsed = SDL_GetTicks() - keyFlashStart_[index];
    return elapsed < 220 && ((elapsed / 55) % 2 == 0);
}

void RetroComputerUI::drawArrowGlyph(SDL_Rect r, VirtualKey key, Color c) const {
    setColor(c);
    const int cx = r.x + r.w / 2;
    const int cy = r.y + r.h / 2;
    const int unit = std::max(2, static_cast<int>(4 * scale_));
    if (key == VirtualKey::ARROW_UP) {
        SDL_Point head[3] = {{cx, cy - 3 * unit}, {cx - 3 * unit, cy}, {cx + 3 * unit, cy}};
        fillPoly(head, 3);
        thickLine(cx - unit, cy, cx + unit, cy, unit);
        thickLine(cx - unit, cy + unit, cx + unit, cy + unit, unit);
        thickLine(cx - unit, cy + 2 * unit, cx + unit, cy + 2 * unit, unit);
    } else if (key == VirtualKey::ARROW_DOWN) {
        SDL_Point head[3] = {{cx, cy + 3 * unit}, {cx - 3 * unit, cy}, {cx + 3 * unit, cy}};
        fillPoly(head, 3);
        thickLine(cx - unit, cy - 2 * unit, cx + unit, cy - 2 * unit, unit);
        thickLine(cx - unit, cy - unit, cx + unit, cy - unit, unit);
        thickLine(cx - unit, cy, cx + unit, cy, unit);
    } else if (key == VirtualKey::ARROW_LEFT) {
        SDL_Point head[3] = {{cx - 3 * unit, cy}, {cx, cy - 3 * unit}, {cx, cy + 3 * unit}};
        fillPoly(head, 3);
        thickLine(cx, cy - unit, cx + 2 * unit, cy - unit, unit);
        thickLine(cx, cy, cx + 2 * unit, cy, unit);
        thickLine(cx, cy + unit, cx + 2 * unit, cy + unit, unit);
    } else if (key == VirtualKey::ARROW_RIGHT) {
        SDL_Point head[3] = {{cx + 3 * unit, cy}, {cx, cy - 3 * unit}, {cx, cy + 3 * unit}};
        fillPoly(head, 3);
        thickLine(cx - 2 * unit, cy - unit, cx, cy - unit, unit);
        thickLine(cx - 2 * unit, cy, cx, cy, unit);
        thickLine(cx - 2 * unit, cy + unit, cx, cy + unit, unit);
    }
}

void RetroComputerUI::drawVirtualKey(VirtualKey key) {
    const int index = keyIndex(key);
    if (index < 0 || index >= virtualKeyCount()) return;
    SDL_Rect r = lay_.virtualKeys[index];
    if (r.w <= 0 || r.h <= 0) return;

    const bool active = isKeyVisuallyActive(index);
    const int depth = lay_.keyDepth;
    const int dx = std::max(1, static_cast<int>(4 * scale_));

    // Fixed-position extrusion: feedback is brightness only, never movement.
    setColor(theme_.keySide);
    fillQuad(r.x, r.y + r.h, r.x + r.w, r.y + r.h,
             r.x + r.w + dx, r.y + r.h + depth,
             r.x + dx, r.y + r.h + depth);
    setColor(theme_.bezelDark);
    fillQuad(r.x + r.w, r.y, r.x + r.w + dx, r.y + depth,
             r.x + r.w + dx, r.y + r.h + depth,
             r.x + r.w, r.y + r.h);

    setColor(active ? theme_.keyTop : theme_.keyFace);
    SDL_RenderFillRect(ren_, &r);
    if (active) {
        setColor(theme_.accent, 120);
        SDL_RenderFillRect(ren_, &r);
    }

    setColor(active ? theme_.accent : theme_.keyTop);
    thickLine(r.x, r.y, r.x + r.w, r.y, std::max(1, static_cast<int>(3 * scale_)));
    thickLine(r.x, r.y, r.x, r.y + r.h, std::max(1, static_cast<int>(2 * scale_)));
    setColor(theme_.keySide);
    thickLine(r.x, r.y + r.h - 2, r.x + r.w, r.y + r.h - 2,
              std::max(1, static_cast<int>(2 * scale_)));
    thickLine(r.x + r.w - 2, r.y, r.x + r.w - 2, r.y + r.h,
              std::max(1, static_cast<int>(2 * scale_)));

    if (key == VirtualKey::ARROW_UP || key == VirtualKey::ARROW_DOWN ||
        key == VirtualKey::ARROW_LEFT || key == VirtualKey::ARROW_RIGHT) {
        drawArrowGlyph(r, key, theme_.keyLabel);
        return;
    }

    const char* label = virtualKeyName(key);
    int textScale = 4;
    if (key == VirtualKey::SAVE || key == VirtualKey::LOAD) textScale = 2;
    if (key == VirtualKey::SPACE) textScale = 3;
    textScale = std::max(1, static_cast<int>(textScale * scale_));
    drawTextCentered(label, r.x + r.w / 2,
                     r.y + (r.h - 5 * textScale) / 2,
                     textScale, theme_.keyLabel);
}

void RetroComputerUI::drawSaveLoad() {
    drawVirtualKey(VirtualKey::SAVE);
    drawVirtualKey(VirtualKey::LOAD);
}

void RetroComputerUI::drawKeyboard() {
    for (VirtualKey key : HEX_KEYPAD) drawVirtualKey(key);
    drawVirtualKey(VirtualKey::ARROW_UP);
    drawVirtualKey(VirtualKey::ARROW_LEFT);
    drawVirtualKey(VirtualKey::ARROW_DOWN);
    drawVirtualKey(VirtualKey::ARROW_RIGHT);
    drawVirtualKey(VirtualKey::SPACE);
}

void RetroComputerUI::beginFrame() {
    drawBackground();
    drawGroundShadow();
    drawComputerBody();
    drawBezelAndGlass();
}

void RetroComputerUI::endFrame() {
    drawCrtEffects();
    drawDecorations();
    drawHardwareDetails();
    drawUpperControls();
    drawWaveformPanel();
    drawSaveLoad();
    drawKeyboard();
}

void RetroComputerUI::render(SDL_Renderer* renderer) {
    if (renderer && renderer != ren_) ren_ = renderer;
    beginFrame();
    renderContent(nullptr);
    endFrame();
    SDL_RenderPresent(ren_);
}

} // namespace retro_gui
