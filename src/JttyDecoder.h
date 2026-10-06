#pragma once

#include <QObject>
#include <QVector>

#include <cstdint>

// Feeds a continuous stream of 12 kHz mono PCM samples to the JTTY decoder
// (thirdparty/jtty_codec) and emits each message once it's fully received.
//
// The underlying Fortran decoder (rjtty_sub_) scans incrementally from where
// it left off each call, keyed off the buffer length growing call to call;
// it only resets its internal sync state when a shorter buffer is passed.
// So samples are appended to an ever-growing buffer rather than a small
// rolling window, with a periodic reset once that buffer gets large enough
// that unbounded growth would matter (see kMaxBufferSeconds in the .cpp).
class JttyDecoder : public QObject
{
    Q_OBJECT

public:
    explicit JttyDecoder(QObject *parent = nullptr);
    ~JttyDecoder() override;

public slots:
    // Appends newly captured 12 kHz mono PCM samples.
    void addSamples(const QVector<int16_t> &samples);

    // Scans whatever's new since the last poll and emits any messages that
    // completed. Call periodically (e.g. every few hundred ms) from a timer.
    void poll();

signals:
    void messageDecoded(QString text, float frequencyHz);

private:
    QVector<int16_t> m_buffer;
};
