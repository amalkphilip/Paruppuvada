#ifndef RETRO_GUI_WAVEFORM_H
#define RETRO_GUI_WAVEFORM_H

#include <cstdint>

namespace retro_gui {

enum class Waveform : uint8_t {
    SQUARE = 0,
    TRIANGLE,
    SINE,
    SAWTOOTH,
    COUNT
};

const char* waveformName(Waveform waveform);
Waveform nextWaveform(Waveform waveform);
Waveform previousWaveform(Waveform waveform);

// phase is normalized to [0, 1). The return value is normalized to [-1, 1].
float waveformSample(Waveform waveform, float phase);

} // namespace retro_gui

#endif // RETRO_GUI_WAVEFORM_H
