// ============================================================================
// RetroComputerUI — implementation. See retro_computer.h for the contract.
//
// Everything is authored in a fixed 960x840 "design space" and uniformly
// scaled + centred into the real window, so every pixel-art coordinate below
// stays a small readable integer.
// ============================================================================

#include "retro_computer.h"

#include <algorithm>
#include <cstring>

namespace {

const int DESIGN_W = 960;
const int DESIGN_H = 840;

// CHIP-8 keypad layout:  1 2 3 C / 4 5 6 D / 7 8 9 E / A 0 B F
// KEY_AT[row*4 + col] = CHIP-8 key index drawn at that grid position.
const int KEY_AT[16] = {
    0x1, 0x2, 0x3, 0xC,
    0x4, 0x5, 0x6, 0xD,
    0x7, 0x8, 0x9, 0xE,
    0xA, 0x0, 0xB, 0xF
};

char keyLabelChar(int key) {
    if (key == 16) return '-';
    if (key == 17) return '+';
    if (key == 18) return 'S';
    if (key == 19) return 'L';
    return "0123456789ABCDEF"[key & 0xF];
}

// ---------------------------------------------------------------------------
// 3x5 pixel font (same bitmap style as the existing emulator UI).
// 15 bits: 5 rows x 3 columns, row 0 in the top bits.
// ---------------------------------------------------------------------------
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
        case '-': return 000700; case '+': return 022722; case '[': return 064446; case ']': return 031113;
        case '.': return 000004; case ' ': return 000000;
        default:  return 000000; // unknown glyphs render as blank
    }
}

RCColor rgb(uint8_t r, uint8_t g, uint8_t b) {
    return RCColor{ r, g, b, 255 };
}

} // namespace

// ---------------------------------------------------------------------------
// Public shared text helper
// ---------------------------------------------------------------------------
void rcui::drawText(SDL_Renderer* renderer, const char* text, int x, int y, int scale) {
    if (!text || scale < 1) return;
    for (const char* p = text; *p; ++p) {
        uint16_t bitmap = glyphFor(*p);
        for (int row = 0; row < 5; ++row) {
            int bits = (bitmap >> (12 - row * 3)) & 7;
            for (int col = 0; col < 3; ++col) {
                if (bits & (4 >> col)) {
                    SDL_Rect px = { x + col * scale, y + row * scale, scale, scale };
                    SDL_RenderFillRect(renderer, &px);
                }
            }
        }
        x += 4 * scale;
    }
}

int rcui::textWidth(const char* text, int scale) {
    if (!text) return 0;
    return (int)std::strlen(text) * 4 * scale;
}

// ---------------------------------------------------------------------------
// Built-in palettes
// ---------------------------------------------------------------------------
RCPaletteDef RetroComputerUI::builtinPalette(RCPalette id) {
    RCPaletteDef p;
    switch (id) {
        case RCPalette::AMBER:
            p.background     = rgb(0x12, 0x0B, 0x04);
            p.backgroundDots = rgb(0x1E, 0x14, 0x08);
            p.bodyDark       = rgb(0x2A, 0x1A, 0x08);
            p.bodyMid        = rgb(0x4A, 0x32, 0x14);
            p.bodyLight      = rgb(0x6B, 0x4C, 0x20);
            p.bodyEdge       = rgb(0x8F, 0x6A, 0x30);
            p.bezel          = rgb(0x33, 0x22, 0x0E);
            p.bezelDark      = rgb(0x1A, 0x10, 0x06);
            p.glass          = rgb(0x14, 0x0C, 0x04);
            p.screenGlow     = rgb(0xFF, 0xB0, 0x30);
            p.keyFace        = rgb(0x5A, 0x3E, 0x18);
            p.keyTop         = rgb(0x8F, 0x6A, 0x30);
            p.keySide        = rgb(0x2A, 0x1A, 0x08);
            p.keyLabel       = rgb(0xFF, 0xC8, 0x60);
            p.accent         = rgb(0xFF, 0xB0, 0x30);
            p.textMain       = rgb(0xFF, 0xC8, 0x60);
            break;
        case RCPalette::PAPER_WHITE:
            p.background     = rgb(0x14, 0x14, 0x12);
            p.backgroundDots = rgb(0x1E, 0x1E, 0x1A);
            p.bodyDark       = rgb(0x3A, 0x3A, 0x34);
            p.bodyMid        = rgb(0x62, 0x62, 0x5A);
            p.bodyLight      = rgb(0x84, 0x84, 0x7A);
            p.bodyEdge       = rgb(0xA8, 0xA8, 0x9C);
            p.bezel          = rgb(0x48, 0x48, 0x42);
            p.bezelDark      = rgb(0x24, 0x24, 0x20);
            p.glass          = rgb(0x18, 0x18, 0x16);
            p.screenGlow     = rgb(0xE8, 0xF0, 0xE0);
            p.keyFace        = rgb(0x74, 0x74, 0x6A);
            p.keyTop         = rgb(0xA8, 0xA8, 0x9C);
            p.keySide        = rgb(0x3A, 0x3A, 0x34);
            p.keyLabel       = rgb(0xF0, 0xF0, 0xE4);
            p.accent         = rgb(0xE8, 0xF0, 0xE0);
            p.textMain       = rgb(0xF0, 0xF0, 0xE4);
            break;
        case RCPalette::GREEN_PHOSPHOR:
        default:
            // The target look: #071108 #0B1F10 #123D1C #39A94A #8FEA9B
            p.background     = rgb(0x07, 0x11, 0x08);
            p.backgroundDots = rgb(0x0B, 0x1F, 0x10);
            p.bodyDark       = rgb(0x0B, 0x1F, 0x10);
            p.bodyMid        = rgb(0x12, 0x3D, 0x1C);
            p.bodyLight      = rgb(0x1E, 0x57, 0x2B);
            p.bodyEdge       = rgb(0x39, 0xA9, 0x4A);
            p.bezel          = rgb(0x0E, 0x2E, 0x16);
            p.bezelDark      = rgb(0x07, 0x11, 0x08);
            p.glass          = rgb(0x08, 0x16, 0x0A);
            p.screenGlow     = rgb(0x39, 0xA9, 0x4A);
            p.keyFace        = rgb(0x1A, 0x4A, 0x24);
            p.keyTop         = rgb(0x39, 0xA9, 0x4A);
            p.keySide        = rgb(0x0B, 0x1F, 0x10);
            p.keyLabel       = rgb(0x8F, 0xEA, 0x9B);
            p.accent         = rgb(0x8F, 0xEA, 0x9B);
            p.textMain       = rgb(0x8F, 0xEA, 0x9B);
            break;
    }
    return p;
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------
RetroComputerUI::RetroComputerUI()
    : ren_(nullptr), winW_(DESIGN_W), winH_(DESIGN_H),
      scale_(1.0f), originX_(0), originY_(0),
      palId_(RCPalette::GREEN_PHOSPHOR), screenOverride_(false),
      mouseKey_(-1), cb_(nullptr), cbUser_(nullptr), speedLevel_(10) {
    pal_ = builtinPalette(palId_);
    std::memset(keyPressed_, 0, sizeof(keyPressed_));
    std::memset(keyFlashStart_, 0, sizeof(keyFlashStart_));
    std::memset(&lay_, 0, sizeof(lay_));
}

RetroComputerUI::~RetroComputerUI() {
    shutdown();
}

bool RetroComputerUI::init(SDL_Renderer* renderer, int windowW, int windowH) {
    if (!renderer) return false;
    ren_ = renderer;

    // Crisp pixel art: no smoothing, alpha blending on.
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    SDL_SetRenderDrawBlendMode(ren_, SDL_BLENDMODE_BLEND);

    computeLayout(windowW, windowH);
    return true;
}

void RetroComputerUI::shutdown() {
    ren_ = nullptr;
}

// ---------------------------------------------------------------------------
// Layout — design space 960x840, uniformly scaled and centred.
// ---------------------------------------------------------------------------
void RetroComputerUI::computeLayout(int w, int h) {
    winW_ = w;
    winH_ = h;
    scale_ = std::min((float)w / (float)DESIGN_W, (float)h / (float)DESIGN_H);
    originX_ = (int)((w - DESIGN_W * scale_) * 0.5f);
    originY_ = (int)((h - DESIGN_H * scale_) * 0.5f);

    auto R = [&](int x, int y, int rw, int rh) {
        SDL_Rect r;
        r.x = originX_ + (int)(x * scale_);
        r.y = originY_ + (int)(y * scale_);
        r.w = std::max(1, (int)(rw * scale_));
        r.h = std::max(1, (int)(rh * scale_));
        return r;
    };
    auto P = [&](int x, int y) {
        SDL_Point pt;
        pt.x = originX_ + (int)(x * scale_);
        pt.y = originY_ + (int)(y * scale_);
        return pt;
    };
    auto S = [&](int v) { return std::max(1, (int)(v * scale_)); };

    // --- monitor section ---------------------------------------------------
    lay_.body   = R(170, 150, 620, 400);
    lay_.depthX = S(44);            // right side face width
    lay_.depthY = -S(32);           // top face rises upward

    // CRT content area: exact 2:1 (matches the 64x32 CHIP-8 framebuffer).
    lay_.screen = R(256, 196, 448, 224);
    lay_.bezel  = inflate(lay_.screen, S(22));
    lay_.glass  = inflate(lay_.screen, S(6));

    // --- keyboard deck (trapezoid sloping toward the viewer) ---------------
    // Back edge tucks directly against the monitor's bottom edge so the
    // keyboard reads as physically attached to the same machine.
    lay_.deck[0]   = P(170, 552);   // back-left  ( = body bottom edge )
    lay_.deck[1]   = P(790, 552);   // back-right
    lay_.deck[2]   = P(810, 784);   // front-right (wider = perspective)
    lay_.deck[3]   = P(150, 784);   // front-left
    lay_.deckFaceH = S(24);

    // --- keypad: 4x4, rows grow slightly toward the viewer -----------------
    lay_.keyDepth = S(6);
    int y = 582;
    for (int row = 0; row < 4; ++row) {
        int kw  = 64 + row * 2;     // subtle perspective growth
        int kh  = 42 + row;
        int gap = 14;
        int rowW = 4 * kw + 3 * gap;
        int x = 480 - rowW / 2;
        for (int col = 0; col < 4; ++col) {
            lay_.key[KEY_AT[row * 4 + col]] = R(x, y, kw, kh);
            x += kw + gap;
        }
        y += kh + 8;
    }
    
    // Speed control buttons to the right of the keypad
    lay_.key[16] = R(680, 582, 42, 42); // SPD-
    lay_.key[17] = R(680, 642, 42, 42); // SPD+

    // Save and Load buttons to the left of the keypad
    lay_.key[18] = R(410, 582, 42, 42); // SAVE
    lay_.key[19] = R(410, 642, 42, 42); // LOAD

    // --- decorations on the monitor "chin" ---------------------------------
    lay_.brandPlate = R(206, 478, 224, 36);
    lay_.cartSlot   = R(470, 488, 196, 14);
    lay_.powerLed   = R(736, 486, 16, 16);
    lay_.speedBar   = R(736, 582, 12, 102);
}

void RetroComputerUI::onWindowResize(int w, int h) {
    computeLayout(w, h);
}

// ---------------------------------------------------------------------------
// Interface 1: content rectangle
// ---------------------------------------------------------------------------
SDL_Rect RetroComputerUI::getScreenArea() const {
    return lay_.screen;
}

void RetroComputerUI::setContentRect(SDL_Rect rect) {
    lay_.screen = rect;
    screenOverride_ = true;
    // keep bezel/glass wrapped around the new area
    int b = std::max(1, (int)(22 * scale_));
    int g = std::max(1, (int)(6 * scale_));
    lay_.bezel = inflate(rect, b);
    lay_.glass = inflate(rect, g);
}

// ---------------------------------------------------------------------------
// Interface 3: keypad visuals
// ---------------------------------------------------------------------------
void RetroComputerUI::setVirtualKeyPressed(int key, bool pressed) {
    if (key >= 0 && key < 18) {
        if (pressed && !keyPressed_[key]) {
            keyFlashStart_[key] = SDL_GetTicks();
        }
        keyPressed_[key] = pressed;
    }
}

bool RetroComputerUI::isVirtualKeyPressed(int key) const {
    return (key >= 0 && key < 20) ? keyPressed_[key] : false;
}

void RetroComputerUI::setSpeedLevel(int speed) {
    speedLevel_ = std::max(1, std::min(40, speed));
}

// ---------------------------------------------------------------------------
// Interface 4: events
// ---------------------------------------------------------------------------
void RetroComputerUI::setKeyCallback(RCKeyCallback cb, void* userdata) {
    cb_ = cb;
    cbUser_ = userdata;
}

int RetroComputerUI::keyAtPoint(int x, int y) const {
    SDL_Point pt{ x, y };
    for (int k = 0; k < 20; ++k) {
        SDL_Rect r = lay_.key[k];
        r.h += lay_.keyDepth; // include the extrusion so clicks feel generous
        if (SDL_PointInRect(&pt, &r)) return k;
    }
    return -1;
}

void RetroComputerUI::handleEvent(const SDL_Event& e) {
    if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
        int k = keyAtPoint(e.button.x, e.button.y);
        if (k >= 0) {
            mouseKey_ = k;
            if (!keyPressed_[k]) {
                keyPressed_[k] = true;
                keyFlashStart_[k] = SDL_GetTicks(); // trigger visual blink
                if (cb_) cb_(k, true, cbUser_);
            }
        }
    } else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
        if (mouseKey_ >= 0) {
            int k = mouseKey_;
            mouseKey_ = -1;
            if (keyPressed_[k]) {
                keyPressed_[k] = false;
                if (cb_) cb_(k, false, cbUser_);
            }
        }
    } else if (e.type == SDL_WINDOWEVENT &&
               e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
        computeLayout(e.window.data1, e.window.data2);
    }
}

// ---------------------------------------------------------------------------
// Interface 5: palette
// ---------------------------------------------------------------------------
void RetroComputerUI::setPalette(RCPalette palette) {
    palId_ = palette;
    pal_ = builtinPalette(palette);
}

void RetroComputerUI::setCustomPalette(const RCPaletteDef& def) {
    pal_ = def;
}

RCPalette RetroComputerUI::getPalette() const {
    return palId_;
}

// ---------------------------------------------------------------------------
// Primitives
// ---------------------------------------------------------------------------
void RetroComputerUI::setColor(RCColor c) const {
    SDL_SetRenderDrawColor(ren_, c.r, c.g, c.b, c.a);
}

void RetroComputerUI::setColor(RCColor c, uint8_t a) const {
    SDL_SetRenderDrawColor(ren_, c.r, c.g, c.b, a);
}

SDL_Rect RetroComputerUI::inflate(SDL_Rect r, int d) {
    SDL_Rect o = { r.x - d, r.y - d, r.w + 2 * d, r.h + 2 * d };
    return o;
}

// Scanline polygon fill — core SDL2 only, no SDL2_gfx, no version worries.
void RetroComputerUI::fillPoly(const SDL_Point* pts, int n) const {
    int minY = pts[0].y, maxY = pts[0].y;
    for (int i = 1; i < n; ++i) {
        minY = std::min(minY, pts[i].y);
        maxY = std::max(maxY, pts[i].y);
    }
    for (int y = minY; y <= maxY; ++y) {
        float xs[8];
        int cnt = 0;
        for (int i = 0; i < n && cnt < 8; ++i) {
            SDL_Point a = pts[i];
            SDL_Point b = pts[(i + 1) % n];
            if ((a.y <= y && b.y > y) || (b.y <= y && a.y > y)) {
                float t = (float)(y - a.y) / (float)(b.y - a.y);
                xs[cnt++] = a.x + t * (b.x - a.x);
            }
        }
        std::sort(xs, xs + cnt);
        for (int i = 0; i + 1 < cnt; i += 2) {
            SDL_RenderDrawLine(ren_, (int)xs[i], y, (int)xs[i + 1], y);
        }
    }
}

void RetroComputerUI::fillQuad(int x0, int y0, int x1, int y1,
                               int x2, int y2, int x3, int y3) const {
    SDL_Point pts[4] = { {x0, y0}, {x1, y1}, {x2, y2}, {x3, y3} };
    fillPoly(pts, 4);
}

void RetroComputerUI::thickLine(int x0, int y0, int x1, int y1, int t) const {
    for (int i = 0; i < t; ++i) {
        if (y0 == y1) SDL_RenderDrawLine(ren_, x0, y0 + i, x1, y1 + i);
        else          SDL_RenderDrawLine(ren_, x0 + i, y0, x1 + i, y1);
    }
}

void RetroComputerUI::drawText(const char* s, int x, int y, int scale, RCColor c) const {
    setColor(c);
    rcui::drawText(ren_, s, x, y, scale);
}

// ---------------------------------------------------------------------------
// Stage: background — dark window + halftone dot grid (reference style)
// ---------------------------------------------------------------------------
void RetroComputerUI::drawBackground() {
    setColor(pal_.background);
    SDL_RenderClear(ren_);

    setColor(pal_.backgroundDots);
    int step = std::max(6, (int)(14 * scale_));
    int dot  = std::max(1, (int)(2 * scale_));
    for (int y = step / 2; y < winH_; y += step) {
        for (int x = step / 2; x < winW_; x += step) {
            SDL_Rect d = { x, y, dot, dot };
            SDL_RenderFillRect(ren_, &d);
        }
    }
}

// ---------------------------------------------------------------------------
// Stage: soft ground shadow under the computer
// ---------------------------------------------------------------------------
void RetroComputerUI::drawGroundShadow() {
    int frontY = lay_.deck[3].y + lay_.deckFaceH;
    int left   = lay_.deck[3].x - (int)(30 * scale_);
    int right  = lay_.deck[2].x + (int)(60 * scale_);
    SDL_Rect base = { left, frontY - (int)(4 * scale_),
                      right - left, (int)(34 * scale_) };
    // nested translucent slabs = cheap pixel-art soft shadow
    const uint8_t a[3] = { 90, 50, 25 };
    for (int i = 0; i < 3; ++i) {
        SDL_Rect r = inflate(base, i * (int)(8 * scale_));
        r.y -= i * (int)(2 * scale_);
        setColor(pal_.bezelDark, a[i]);
        SDL_RenderFillRect(ren_, &r);
    }
}

// ---------------------------------------------------------------------------
// Stage: computer body — top face, right side face, front face, deck
// ---------------------------------------------------------------------------
void RetroComputerUI::drawComputerBody() {
    const SDL_Rect& b = lay_.body;
    int dx = lay_.depthX, dy = lay_.depthY;

    // top face (lit by the imaginary ceiling light)
    setColor(pal_.bodyLight);
    fillQuad(b.x, b.y,
             b.x + b.w, b.y,
             b.x + b.w + dx, b.y + dy,
             b.x + dx, b.y + dy);
    // bright rim along the top-back edge
    setColor(pal_.bodyEdge);
    thickLine(b.x + dx, b.y + dy, b.x + b.w + dx, b.y + dy, std::max(1, (int)(2 * scale_)));

    // right side face (in shadow)
    setColor(pal_.bodyDark);
    fillQuad(b.x + b.w, b.y,
             b.x + b.w + dx, b.y + dy,
             b.x + b.w + dx, b.y + b.h + dy,
             b.x + b.w, b.y + b.h);
    // side-face shading: darker toward the back
    setColor(pal_.bezelDark, 60);
    fillQuad(b.x + b.w + dx / 2, b.y + dy / 2,
             b.x + b.w + dx, b.y + dy,
             b.x + b.w + dx, b.y + b.h + dy,
             b.x + b.w + dx / 2, b.y + b.h + dy / 2);

    // vents on the right side face
    setColor(pal_.bezelDark);
    for (int i = 0; i < 5; ++i) {
        int vy = b.y + (int)((60 + i * 52) * scale_);
        fillQuad(b.x + b.w + (int)(8 * scale_), vy,
                 b.x + b.w + dx - (int)(8 * scale_), vy + dy / 2,
                 b.x + b.w + dx - (int)(8 * scale_), vy + dy / 2 + (int)(6 * scale_),
                 b.x + b.w + (int)(8 * scale_), vy + (int)(6 * scale_));
    }

    // front face
    setColor(pal_.bodyMid);
    SDL_RenderFillRect(ren_, &b);
    // lit top + left edges, shaded bottom edge
    setColor(pal_.bodyEdge);
    thickLine(b.x, b.y, b.x + b.w, b.y, std::max(1, (int)(3 * scale_)));
    thickLine(b.x, b.y, b.x, b.y + b.h, std::max(1, (int)(2 * scale_)));
    setColor(pal_.bodyDark);
    thickLine(b.x, b.y + b.h - std::max(1, (int)(3 * scale_)), b.x + b.w,
              b.y + b.h - std::max(1, (int)(3 * scale_)), std::max(1, (int)(3 * scale_)));

    // --- keyboard deck -----------------------------------------------------
    const SDL_Point* d = lay_.deck;

    // deck right side face
    setColor(pal_.bodyDark);
    fillQuad(d[1].x, d[1].y, d[2].x, d[2].y,
             d[2].x, d[2].y + lay_.deckFaceH, d[1].x, d[1].y + lay_.deckFaceH / 2);
    // deck front face (thickness)
    setColor(pal_.bodyDark);
    fillQuad(d[3].x, d[3].y, d[2].x, d[2].y,
             d[2].x, d[2].y + lay_.deckFaceH, d[3].x, d[3].y + lay_.deckFaceH);
    // deck top surface
    setColor(pal_.bodyMid);
    fillQuad(d[0].x, d[0].y, d[1].x, d[1].y, d[2].x, d[2].y, d[3].x, d[3].y);
    // deck shading: darker toward the front edge
    setColor(pal_.bezelDark, 22);
    fillQuad(d[3].x, d[3].y - (int)(44 * scale_), d[2].x, d[2].y - (int)(44 * scale_),
             d[2].x, d[2].y, d[3].x, d[3].y);
    // dark seam where the deck joins the monitor case
    setColor(pal_.bezelDark, 110);
    thickLine(d[0].x, d[0].y + 1, d[1].x, d[1].y + 1, std::max(1, (int)(2 * scale_)));
    // lit back edge where deck meets the monitor
    setColor(pal_.bodyEdge);
    thickLine(d[0].x, d[0].y, d[1].x, d[1].y, std::max(1, (int)(2 * scale_)));
    // lit front lip
    setColor(pal_.bodyLight);
    thickLine(d[3].x, d[3].y, d[2].x, d[2].y, std::max(1, (int)(2 * scale_)));

    // corner screws on the front face
    int s = std::max(2, (int)(4 * scale_));
    int m = (int)(12 * scale_);
    SDL_Point screws[4] = {
        { b.x + m, b.y + m }, { b.x + b.w - m, b.y + m },
        { b.x + m, b.y + b.h - m }, { b.x + b.w - m, b.y + b.h - m }
    };
    for (int i = 0; i < 4; ++i) {
        setColor(pal_.bodyDark);
        SDL_Rect sq = { screws[i].x - s / 2, screws[i].y - s / 2, s, s };
        SDL_RenderFillRect(ren_, &sq);
        setColor(pal_.bodyEdge);
        SDL_RenderDrawLine(ren_, sq.x, sq.y, sq.x + sq.w - 1, sq.y);
    }
}

// ---------------------------------------------------------------------------
// Stage: CRT bezel + recessed glass (content goes in later, on top of glass)
// ---------------------------------------------------------------------------
void RetroComputerUI::drawBezelAndGlass() {
    const SDL_Rect& bz = lay_.bezel;
    const SDL_Rect& gl = lay_.glass;

    // bezel slab
    setColor(pal_.bezel);
    SDL_RenderFillRect(ren_, &bz);

    // bezel outer bevel: lit top/left, shaded bottom/right
    setColor(pal_.bodyLight);
    thickLine(bz.x, bz.y, bz.x + bz.w, bz.y, std::max(1, (int)(3 * scale_)));
    thickLine(bz.x, bz.y, bz.x, bz.y + bz.h, std::max(1, (int)(3 * scale_)));
    setColor(pal_.bezelDark);
    thickLine(bz.x, bz.y + bz.h - 2, bz.x + bz.w, bz.y + bz.h - 2, std::max(1, (int)(3 * scale_)));
    thickLine(bz.x + bz.w - 2, bz.y, bz.x + bz.w - 2, bz.y + bz.h, std::max(1, (int)(3 * scale_)));

    // recess: the glass sits *into* the bezel — dark inner top/left shadow,
    // faint light catching the inner bottom/right lip.
    setColor(pal_.bezelDark);
    thickLine(gl.x - 3, gl.y - 3, gl.x + gl.w + 3, gl.y - 3, std::max(1, (int)(4 * scale_)));
    thickLine(gl.x - 3, gl.y - 3, gl.x - 3, gl.y + gl.h + 3, std::max(1, (int)(4 * scale_)));
    setColor(pal_.bodyLight, 120);
    thickLine(gl.x - 1, gl.y + gl.h + 1, gl.x + gl.w + 1, gl.y + gl.h + 1, std::max(1, (int)(2 * scale_)));
    thickLine(gl.x + gl.w + 1, gl.y - 1, gl.x + gl.w + 1, gl.y + gl.h + 1, std::max(1, (int)(2 * scale_)));

    // unlit glass (your content is drawn over this in renderContent)
    setColor(pal_.glass);
    SDL_RenderFillRect(ren_, &gl);
}

// ---------------------------------------------------------------------------
// Interface 2, middle step: blit the emulator content into the CRT area
// ---------------------------------------------------------------------------
void RetroComputerUI::renderContent(SDL_Texture* content) {
    // phosphor glow bleeding onto the glass around the picture
    setColor(pal_.screenGlow, 22);
    SDL_RenderFillRect(ren_, &lay_.glass);

    if (content) {
        SDL_RenderCopy(ren_, content, nullptr, &lay_.screen);
    }
}

// ---------------------------------------------------------------------------
// Stage: CRT effects drawn OVER the emulator content
// ---------------------------------------------------------------------------
void RetroComputerUI::drawCrtEffects() {
    const SDL_Rect& sc = lay_.screen;
    SDL_Rect clip = lay_.glass;
    SDL_RenderSetClipRect(ren_, &clip);

    // scanlines
    int lineStep = std::max(2, (int)(2 * scale_));
    int lineH    = std::max(1, (int)(1 * scale_));
    setColor(pal_.bezelDark, 46);
    for (int y = sc.y; y < sc.y + sc.h; y += lineStep * 2) {
        SDL_Rect l = { sc.x, y, sc.w, lineH };
        SDL_RenderFillRect(ren_, &l);
    }

    // subtle brightness flicker
    uint8_t fa = (uint8_t)(6 + ((SDL_GetTicks() / 90) % 2) * 4);
    setColor(pal_.screenGlow, fa);
    SDL_RenderFillRect(ren_, &sc);

    // glass reflection streak (top-left diagonal)
    setColor(pal_.accent, 14);
    fillQuad(sc.x + (int)(14 * scale_), sc.y,
             sc.x + sc.w / 3, sc.y,
             sc.x + sc.w / 6, sc.y + sc.h,
             sc.x - (int)(30 * scale_), sc.y + sc.h);

    // corner vignette — fakes the curved CRT glass (kept subtle)
    int v = std::max(2, (int)(6 * scale_));
    setColor(pal_.bezelDark, 45);
    for (int i = 0; i < 3; ++i) {
        int o = i * v / 2;
        int t = v - i * 2;
        if (t < 1) break;
        SDL_Rect tl = { sc.x + o, sc.y + o, v * 2, t }; SDL_RenderFillRect(ren_, &tl);
        SDL_Rect tr = { sc.x + sc.w - o - v * 2, sc.y + o, v * 2, t }; SDL_RenderFillRect(ren_, &tr);
        SDL_Rect bl = { sc.x + o, sc.y + sc.h - o - t, v * 2, t }; SDL_RenderFillRect(ren_, &bl);
        SDL_Rect br = { sc.x + sc.w - o - v * 2, sc.y + sc.h - o - t, v * 2, t }; SDL_RenderFillRect(ren_, &br);
    }

    // dark border ring between glass and picture
    setColor(pal_.bezelDark);
    SDL_Rect inner = lay_.glass;
    for (int i = 0; i < std::max(1, (int)(3 * scale_)); ++i) {
        SDL_RenderDrawRect(ren_, &inner);
        inner = inflate(inner, -1);
    }

    SDL_RenderSetClipRect(ren_, nullptr);
}

// ---------------------------------------------------------------------------
// Stage: decorations — brand plate, cartridge slot, power LED
// ---------------------------------------------------------------------------
void RetroComputerUI::drawDecorations() {
    // brand plate
    const SDL_Rect& bp = lay_.brandPlate;
    setColor(pal_.bodyDark);
    SDL_RenderFillRect(ren_, &bp);
    setColor(pal_.bodyEdge, 140);
    SDL_RenderDrawRect(ren_, &bp);
    const char* brand = "PARUPPUVADA PC-8";
    int ts = std::max(1, (int)(2 * scale_));
    int tw = rcui::textWidth(brand, ts);
    drawText(brand, bp.x + (bp.w - tw) / 2, bp.y + (bp.h - 5 * ts) / 2, ts, pal_.textMain);

    // cartridge slot
    const SDL_Rect& cs = lay_.cartSlot;
    setColor(pal_.bezelDark);
    SDL_RenderFillRect(ren_, &cs);
    setColor(pal_.bodyLight, 160);
    thickLine(cs.x, cs.y + cs.h, cs.x + cs.w, cs.y + cs.h, std::max(1, (int)(1 * scale_)));

    // power LED + glow
    const SDL_Rect& led = lay_.powerLed;
    setColor(pal_.accent, 36);
    SDL_Rect glow = inflate(led, (int)(5 * scale_));
    SDL_RenderFillRect(ren_, &glow);
    setColor(pal_.accent);
    SDL_RenderFillRect(ren_, &led);
    setColor(pal_.textMain, 200);
    thickLine(led.x, led.y, led.x + led.w, led.y, std::max(1, (int)(2 * scale_)));
    
    // speed bar
    const SDL_Rect& sb = lay_.speedBar;
    setColor(pal_.bodyDark);
    SDL_RenderFillRect(ren_, &sb);
    setColor(pal_.bodyEdge, 140);
    SDL_RenderDrawRect(ren_, &sb);
    
    int leds_to_light = std::max(1, std::min(20, (speedLevel_ + 1) / 2));
    for (int i = 0; i < 20; i++) {
        SDL_Rect ledR = { sb.x + 2, sb.y + sb.h - 2 - (i * 5) - 4, sb.w - 4, 4 };
        if (i < leds_to_light) {
            setColor(pal_.accent); // lit
        } else {
            setColor(pal_.glass); // unlit
        }
        SDL_RenderFillRect(ren_, &ledR);
    }
}

// ---------------------------------------------------------------------------
// Stage: virtual CHIP-8 keypad
// ---------------------------------------------------------------------------
void RetroComputerUI::drawKey(int key) {
    SDL_Rect r = lay_.key[key];
    bool pressed = keyPressed_[key];
    
    // Non-blocking pulse/blink logic
    bool active_flash = false;
    uint32_t now = SDL_GetTicks();
    uint32_t elapsed = now - keyFlashStart_[key];
    if (elapsed < 200) { // 200ms total flash duration
        // Flash bright for 50ms, normal for 50ms, bright for 50ms, normal for 50ms
        active_flash = (elapsed / 50) % 2 == 0;
    }
    
    bool visually_active = pressed || active_flash;

    int depth = lay_.keyDepth; // Key no longer moves downward
    int dx = std::max(1, (int)(4 * scale_));

    // extruded thickness: bottom face + right face
    setColor(pal_.keySide);
    fillQuad(r.x, r.y + r.h, r.x + r.w, r.y + r.h,
             r.x + r.w + dx, r.y + r.h + depth, r.x + dx, r.y + r.h + depth);
    setColor(pal_.bezelDark);
    fillQuad(r.x + r.w, r.y, r.x + r.w + dx, r.y + depth,
             r.x + r.w + dx, r.y + r.h + depth, r.x + r.w, r.y + r.h);

    if (visually_active) {
        // bright/active key state
        setColor(pal_.bodyLight); // Use the lightest color for a bright "glow" effect
        SDL_RenderFillRect(ren_, &r);
        
        // draw highlights still, so it doesn't look flat
        setColor(pal_.accent);
        thickLine(r.x, r.y, r.x + r.w, r.y, std::max(1, (int)(3 * scale_)));
        thickLine(r.x, r.y, r.x, r.y + r.h, std::max(1, (int)(2 * scale_)));
    } else {
        // normal top face
        setColor(pal_.keyFace);
        SDL_RenderFillRect(ren_, &r);

        // raised key: lit top + left bevel, shaded bottom + right bevel
        setColor(pal_.keyTop);
        thickLine(r.x, r.y, r.x + r.w, r.y, std::max(1, (int)(3 * scale_)));
        thickLine(r.x, r.y, r.x, r.y + r.h, std::max(1, (int)(2 * scale_)));
        setColor(pal_.keySide);
        thickLine(r.x, r.y + r.h - 2, r.x + r.w, r.y + r.h - 2, std::max(1, (int)(2 * scale_)));
        thickLine(r.x + r.w - 2, r.y, r.x + r.w - 2, r.y + r.h, std::max(1, (int)(2 * scale_)));
    }

    // label
    char label[2] = { keyLabelChar(key), '\0' };
    int ts = std::max(1, (int)(4 * scale_));
    int tw = rcui::textWidth(label, ts);
    drawText(label, r.x + (r.w - tw) / 2, r.y + (r.h - 5 * ts) / 2, ts, pal_.keyLabel);
}

void RetroComputerUI::drawKeypad() {
    for (int k = 0; k < 20; ++k) drawKey(k);
}

// ---------------------------------------------------------------------------
// Interface 2: frame pipeline
// ---------------------------------------------------------------------------
void RetroComputerUI::beginFrame() {
    drawBackground();
    drawGroundShadow();
    drawComputerBody();
    drawBezelAndGlass();
}

void RetroComputerUI::endFrame() {
    drawCrtEffects();
    drawDecorations();
    drawKeypad();
    SDL_RenderPresent(ren_);
}
