#include "SpectrumWidget.h"

#include <QPainter>

#include <algorithm>
#include <cstring>

namespace {
constexpr int kHeight = 90;      // waterfall rows kept ~= seconds of history at the update rate
// Display range; actual signal + noise floor levels vary a lot by sound
// card and mic gain, so these are just reasonable defaults for "something
// is there" visibility rather than a calibrated reading.
constexpr float kMinDb = -90.0f;
constexpr float kMaxDb = -20.0f;
constexpr int kDefaultToneHz = 1500; // matches Jtty::defaultToneHz

struct ColorStop
{
    float position;
    int r, g, b;
};

// Dark blue (quiet) -> blue -> teal -> yellow -> red (loud), a simple
// hand-rolled stand-in for a classic SDR waterfall colormap.
constexpr ColorStop kColorStops[] = {
    {0.00f, 8, 10, 30},
    {0.35f, 20, 40, 170},
    {0.55f, 30, 180, 170},
    {0.75f, 230, 210, 40},
    {1.00f, 235, 40, 30},
};

QRgb colorForMagnitude(float db)
{
    const float t = std::clamp((db - kMinDb) / (kMaxDb - kMinDb), 0.0f, 1.0f);

    int i = 0;
    while (i < int(std::size(kColorStops)) - 2 && t > kColorStops[i + 1].position)
        ++i;
    const ColorStop &a = kColorStops[i];
    const ColorStop &b = kColorStops[i + 1];
    const float span = b.position - a.position;
    const float localT = span > 0.0f ? (t - a.position) / span : 0.0f;

    const int r = a.r + int((b.r - a.r) * localT);
    const int g = a.g + int((b.g - a.g) * localT);
    const int blue = a.b + int((b.b - a.b) * localT);
    return qRgb(r, g, blue);
}
}

SpectrumWidget::SpectrumWidget(int lowHz, int highHz, QWidget *parent)
    : QWidget(parent)
    , m_lowHz(lowHz)
    , m_highHz(highHz)
{
    setFixedHeight(kHeight);
}

QSize SpectrumWidget::sizeHint() const
{
    return {200, kHeight};
}

void SpectrumWidget::setMagnitudesDb(const QVector<float> &magnitudesDb)
{
    const int n = magnitudesDb.size();
    if (n < 1)
        return;

    if (m_waterfall.isNull() || m_waterfall.width() != n) {
        m_waterfall = QImage(n, kHeight, QImage::Format_RGB32);
        m_waterfall.fill(colorForMagnitude(kMinDb));
    }

    // Shift every row down by one (memmove, since source and destination
    // overlap), then paint the newest spectrum into the now-free top row.
    uchar *bits = m_waterfall.bits();
    const int stride = m_waterfall.bytesPerLine();
    std::memmove(bits + stride, bits, size_t(stride) * (kHeight - 1));

    QRgb *topRow = reinterpret_cast<QRgb *>(bits);
    for (int i = 0; i < n; ++i)
        topRow[i] = colorForMagnitude(magnitudesDb[i]);

    update();
}

void SpectrumWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor(8, 10, 30));

    if (!m_waterfall.isNull()) {
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.drawImage(rect(), m_waterfall);
    }

    // Marker at the default JTTY tone, so it's obvious where to look.
    if (m_lowHz <= kDefaultToneHz && kDefaultToneHz <= m_highHz) {
        const int w = width();
        const int h = height();
        const float x = w * float(kDefaultToneHz - m_lowHz) / float(m_highHz - m_lowHz);
        painter.setPen(QPen(QColor(255, 255, 255, 70), 1, Qt::DashLine));
        painter.drawLine(QPointF(x, 0), QPointF(x, h));
    }
}
