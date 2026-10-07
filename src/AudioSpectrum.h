#pragma once

#include <QVector>

#include <cstdint>

// Computes a short-time magnitude spectrum (in dB) over a frequency band
// from a rolling window of the most recently fed samples. Used to drive
// SpectrumWidget from live capture audio.
class AudioSpectrum
{
public:
    AudioSpectrum(int fftSize, int sampleRate);
    ~AudioSpectrum();

    AudioSpectrum(const AudioSpectrum &) = delete;
    AudioSpectrum &operator=(const AudioSpectrum &) = delete;

    // Appends samples to the rolling window. Once enough new samples have
    // accumulated since the last update, recomputes the FFT and fills
    // magnitudesDb with one value per bin spanning [lowHz, highHz],
    // returning true; otherwise leaves magnitudesDb untouched and returns
    // false.
    bool addSamples(const QVector<int16_t> &samples, int lowHz, int highHz,
                     QVector<float> &magnitudesDb);

private:
    struct Impl;
    Impl *m_impl;
};
