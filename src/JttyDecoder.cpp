#include "JttyDecoder.h"
#include "JttyCodec.h"

#include <array>

namespace {
using fortran_charlen_t = long;
constexpr int kBatchSize = 30;
constexpr int kMessageLength = 80;

// Audio search band and default listening tone/tolerance, matching WSJT-X's
// own JTTY defaults (RxFreqSpinBox_2 / sbFtol_2) so two default-configured
// stations can hear each other without any frequency UI on our side.
constexpr int kSearchBandLowHz = 200;
constexpr int kSearchBandHighHz = 2800;
constexpr float kListenToleranceHz = 20.0f;

// ~10 minutes at 12 kHz; large enough that resets are rare in normal use,
// small enough to keep memory bounded for a long-running session.
constexpr qsizetype kMaxBufferSamples = 10 * 60 * Jtty::rxSampleRate;
}

extern "C" {
void rjtty_sub_(short int d2[], int *k, int *nsps, int *nfa, int *nfb, float *f0, float *ftol);
void jtty_get_updates_(char text_blocks[], qint64 message_ids[], float frequencies[],
                        float start_tsync[], bool eom[], int *count, fortran_charlen_t);
void jtty_release_fft_resources();
}

JttyDecoder::JttyDecoder(QObject *parent)
    : QObject(parent)
{
}

JttyDecoder::~JttyDecoder()
{
    jtty_release_fft_resources();
}

void JttyDecoder::addSamples(const QVector<int16_t> &samples)
{
    if (m_buffer.size() + samples.size() > kMaxBufferSamples)
        m_buffer.clear();
    m_buffer.append(samples);
}

void JttyDecoder::poll()
{
    if (m_buffer.isEmpty())
        return;

    int k = static_cast<int>(m_buffer.size());
    int nsps = Jtty::rxSamplesPerSymbol;
    int nfa = kSearchBandLowHz;
    int nfb = kSearchBandHighHz;
    float f0 = Jtty::defaultToneHz;
    float ftol = kListenToleranceHz;
    rjtty_sub_(reinterpret_cast<short int *>(m_buffer.data()), &k, &nsps, &nfa, &nfb, &f0, &ftol);

    int count = 0;
    do {
        std::array<char, kBatchSize * kMessageLength> textBlocks{};
        std::array<qint64, kBatchSize> messageIds{};
        std::array<float, kBatchSize> frequencies{};
        std::array<float, kBatchSize> sequenceStarts{};
        std::array<bool, kBatchSize> complete{};
        jtty_get_updates_(textBlocks.data(), messageIds.data(), frequencies.data(),
                           sequenceStarts.data(), complete.data(), &count,
                           (fortran_charlen_t)textBlocks.size());

        for (int i = 0; i < count; ++i) {
            if (messageIds[i] <= 0)
                continue;
            QString const text =
                QString::fromLatin1(textBlocks.data() + i * kMessageLength, kMessageLength)
                    .trimmed();
            if (!text.isEmpty())
                Q_EMIT messageUpdated(messageIds[i], text, frequencies[i], complete[i]);
        }
    } while (count == kBatchSize);
}
