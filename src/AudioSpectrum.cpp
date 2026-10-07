#include "AudioSpectrum.h"

#include <fftw3.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace {
// Recompute roughly every 40ms at 12kHz rather than on every single sample.
constexpr int kUpdateHopSamples = 480;
}

struct AudioSpectrum::Impl
{
    int fftSize;
    int sampleRate;
    std::vector<float> ring;
    int ringPos = 0;
    int samplesSinceUpdate = 0;
    std::vector<float> window;
    std::vector<float> in;
    fftwf_complex *out;
    fftwf_plan plan;
};

AudioSpectrum::AudioSpectrum(int fftSize, int sampleRate)
    : m_impl(new Impl)
{
    m_impl->fftSize = fftSize;
    m_impl->sampleRate = sampleRate;
    m_impl->ring.assign(fftSize, 0.0f);
    m_impl->window.resize(fftSize);
    for (int i = 0; i < fftSize; ++i)
        m_impl->window[i] = 0.5f - 0.5f * std::cos(2.0f * float(M_PI) * i / float(fftSize - 1));
    m_impl->in.resize(fftSize);
    m_impl->out = fftwf_alloc_complex(fftSize / 2 + 1);
    m_impl->plan = fftwf_plan_dft_r2c_1d(fftSize, m_impl->in.data(), m_impl->out, FFTW_ESTIMATE);
}

AudioSpectrum::~AudioSpectrum()
{
    fftwf_destroy_plan(m_impl->plan);
    fftwf_free(m_impl->out);
    delete m_impl;
}

bool AudioSpectrum::addSamples(const QVector<int16_t> &samples, int lowHz, int highHz,
                                QVector<float> &magnitudesDb)
{
    const int fftSize = m_impl->fftSize;
    bool dueForUpdate = false;

    for (int16_t sample : samples) {
        m_impl->ring[m_impl->ringPos] = float(sample) / 32768.0f;
        m_impl->ringPos = (m_impl->ringPos + 1) % fftSize;
        if (++m_impl->samplesSinceUpdate >= kUpdateHopSamples) {
            m_impl->samplesSinceUpdate = 0;
            dueForUpdate = true;
        }
    }
    if (!dueForUpdate)
        return false;

    for (int i = 0; i < fftSize; ++i) {
        const int idx = (m_impl->ringPos + i) % fftSize;
        m_impl->in[i] = m_impl->ring[idx] * m_impl->window[i];
    }
    fftwf_execute(m_impl->plan);

    const float binHz = float(m_impl->sampleRate) / float(fftSize);
    const int maxBin = fftSize / 2;
    const int loBin = std::clamp(int(std::floor(lowHz / binHz)), 0, maxBin);
    const int hiBin = std::clamp(int(std::ceil(highHz / binHz)), 0, maxBin);

    magnitudesDb.clear();
    magnitudesDb.reserve(hiBin - loBin + 1);
    for (int bin = loBin; bin <= hiBin; ++bin) {
        const float re = m_impl->out[bin][0];
        const float im = m_impl->out[bin][1];
        const float magnitude = std::sqrt(re * re + im * im) / float(fftSize);
        magnitudesDb.append(20.0f * std::log10(magnitude + 1.0e-9f));
    }
    return true;
}
