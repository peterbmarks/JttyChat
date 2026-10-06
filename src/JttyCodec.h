#pragma once

#include <QString>
#include <QVector>

#include <cstdint>

// Thin C++ wrapper around the JTTY mode codec (thirdparty/jtty_codec, copied
// from wsjtx-3.2.0-rc1's lib/jtty). See lib/jtty/jtty_design.md in that
// source tree for the wire format this implements.
namespace Jtty {

// Wire parameters fixed by the codec (see jtty_design.md "Frame and
// waveform"): 12000/384 = 31.25 baud, frames are 59 symbols long.
constexpr int rxSampleRate = 12000;
constexpr int rxSamplesPerSymbol = 384;
constexpr int txSampleRate = 48000;
constexpr int txSamplesPerSymbol = 4 * rxSamplesPerSymbol;
constexpr int symbolsPerFrame = 59;
constexpr int maxMessageLength = 80;
constexpr int maxFrames = 16; // genjtty's MAX_TONES = 59*16

// Default audio tone (Hz) used for both transmit and the decoder's primary
// search channel, matching WSJT-X's own JTTY defaults (RxFreqSpinBox_2 /
// TxFreqSpinBox_2) so two default-configured stations can hear each other.
constexpr float defaultToneHz = 1500.0f;

struct EncodedMessage
{
    bool ok = false;
    QString canonicalText;     // text as actually encoded (whitespace-padded input, trimmed)
    QVector<int16_t> samples;  // 48 kHz mono PCM, ready to play
};

// Encodes message for transmission at the given audio tone frequency (Hz).
// Truncates to maxMessageLength and replaces unsupported characters with
// '#', matching the codec's own text normalization.
EncodedMessage encodeMessage(const QString &message, float toneFrequencyHz);

} // namespace Jtty
