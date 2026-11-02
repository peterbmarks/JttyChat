#include "JttyCodec.h"

#include <algorithm>
#include <cmath>
#include <cstring>

extern "C" {
// lib/jtty/genjtty.f90 (bind(C)). Rewrites msg, an 80-character field, as the
// text its frames carry. is_final=0 leaves the end-of-message flag clear.
// status is JTTY_ENCODE_OK (0) on success.
void genjtty_text_c(char msg[], int exchange_profile, int is_final, int itone[], int *nsym,
                   char frames[], int *nframes, int frame_starts[], int *status);
void gen_jttywave_(int itone[], int *nsym, int *nsps, float *bt, float *fsample, float *f0,
                    float xjunk[], float wave[], int *icmplx, int *nwave);
}

namespace Jtty {

namespace {

// Matches the codec's own alphabet (lib/jtty/jtty_source_codec.f90's
// ALPHABET), plus lowercase a-z which the codec also accepts.
bool isSupportedCharacter(QChar c)
{
    static const QString alphabet =
        QStringLiteral("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ +-./?!\"#$%,&*()_'=[]{}<>|:;");
    return alphabet.contains(c) || (c >= QLatin1Char('a') && c <= QLatin1Char('z'));
}

// Builds the fixed-width, space-padded 80-character frame genjtty_text_c
// expects. As upstream's prepareTransmitText: '~' (the receiver's missed-frame
// sentinel) and NUL become spaces, line breaks become spaces (we send a single
// segment), and anything else the codec can't represent becomes '#'.
QByteArray prepareFrame(const QString &message)
{
    const QString bounded = message.left(maxMessageLength);
    QString sanitized;
    sanitized.reserve(bounded.size());
    for (QChar c : bounded) {
        if (c == QChar(0) || c == QLatin1Char('~') || c == QLatin1Char('\n')
            || c == QLatin1Char('\r'))
            sanitized.append(QLatin1Char(' '));
        else
            sanitized.append(isSupportedCharacter(c) ? c : QLatin1Char('#'));
    }

    QByteArray frame = sanitized.toLatin1();
    frame.resize(maxMessageLength, ' ');
    return frame;
}

} // namespace

EncodedMessage encodeMessage(const QString &message, float toneFrequencyHz)
{
    EncodedMessage result;

    QByteArray frame = prepareFrame(message);
    if (frame.trimmed().isEmpty())
        return result;

    int itone[symbolsPerFrame * maxFrames];
    char packedFrames[frameBits * maxFrames];
    int frameStarts[maxFrames] = {};
    int nsym = 0;
    int nframes = 0;
    int status = -1;
    int const exchangeProfile = 0; // NativeExchangeProfile::None - no contest exchange UI here
    int const isFinal = 1;         // single segment: always carries the end-of-message flag
    genjtty_text_c(frame.data(), exchangeProfile, isFinal, itone, &nsym, packedFrames, &nframes,
                   frameStarts, &status);
    if (status != 0 || nsym <= 0)
        return result;

    // genjtty_text_c normalizes the message in place (padding/casing);
    // read it back so what we display/log matches what was actually sent.
    result.canonicalText = QString::fromLatin1(frame).trimmed();

    int nsps = txSamplesPerSymbol;
    float bt = 2.0f;
    float fsample = static_cast<float>(txSampleRate);
    float f0 = toneFrequencyHz;
    int icmplx = 0;
    int nwave = nsym * nsps;

    QVector<float> wave(nwave);
    gen_jttywave_(itone, &nsym, &nsps, &bt, &fsample, &f0, wave.data(), wave.data(), &icmplx,
                  &nwave);
    if (nwave <= 0)
        return result;

    result.samples.resize(nwave);
    for (int i = 0; i < nwave; ++i) {
        float v = wave[i] * 32767.0f;
        v = std::clamp(v, -32768.0f, 32767.0f);
        result.samples[i] = static_cast<int16_t>(std::lround(v));
    }

    result.ok = true;
    return result;
}

} // namespace Jtty
