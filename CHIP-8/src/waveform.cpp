#include "waveform.h"

#include <algorithm>
#include <cmath>

namespace retro_gui {

namespace {
constexpr float PI = 3.14159265358979323846f;

float wrapPhase(float phase) {
    phase = std::fmod(phase, 1.0f);
    if (phase < 0.0f) phase += 1.0f;
    return phase;
}
} // namespace

const char* waveformName(Waveform waveform) {
    switch (waveform) {
        case Waveform::SQUARE: return "SQUARE";
        case Waveform::TRIANGLE: return "TRIANGLE";
        case Waveform::SINE: return "SINE";
        case Waveform::SAWTOOTH: return "SAWTOOTH";
        case Waveform::COUNT: break;
    }
    return "UNKNOWN";
}

Waveform nextWaveform(Waveform waveform) {
    const int count = static_cast<int>(Waveform::COUNT);
    return static_cast<Waveform>((static_cast<int>(waveform) + 1) % count);
}

Waveform previousWaveform(Waveform waveform) {
    const int count = static_cast<int>(Waveform::COUNT);
    return static_cast<Waveform>((static_cast<int>(waveform) + count - 1) % count);
}

float waveformSample(Waveform waveform, float phase) {
    const float p = wrapPhase(phase);
    switch (waveform) {
        case Waveform::SQUARE:
            return p < 0.5f ? 1.0f : -1.0f;
        case Waveform::TRIANGLE:
            return p < 0.5f ? (4.0f * p - 1.0f) : (3.0f - 4.0f * p);
        case Waveform::SINE:
            return std::sin(2.0f * PI * p);
        case Waveform::SAWTOOTH:
            return 2.0f * p - 1.0f;
        case Waveform::COUNT:
            break;
    }
    return 0.0f;
}

} // namespace retro_gui
