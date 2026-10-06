#include "JttyCodec.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace {
using fortran_charlen_t = long;
}

extern "C" {
void genjtty_profile_(char *msg, const int *exchange_profile, int itone[], int *nsym,
                       fortran_charlen_t);
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

// Builds the fixed-width, space-padded 80-character frame genjtty_profile_
// expects, sanitizing anything the codec can't represent to '#'.
QByteArray prepareFrame(const QString &message)
{
    const QString bounded = message.left(maxMessageLength);
    QString sanitized;
    sanitized.reserve(bounded.size());
    for (QChar c : bounded)
        sanitized.append(isSupportedCharacter(c) ? c : QLatin1Char('#'));

    QByteArray frame = sanitized.toLatin1();
    frame.resize(maxMessageLength, ' ');
    return frame;
}

} // namespace

EncodedMessage encodeMessage(const QString &message, float toneFrequencyHz)
{
    EncodedMessage result;

    QByteArray frame = prepareFrame(message);
    int itone[symbolsPerFrame * maxFrames];
    int nsym = 0;
    int const exchangeProfile = 0; // NativeExchangeProfile::None - no contest exchange UI here
    genjtty_profile_(frame.data(), &exchangeProfile, itone, &nsym, maxMessageLength);
    if (nsym <= 0)
        return result;

    // genjtty_profile_ normalizes the message in place (padding/casing);
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
