#include "SpectrumWidget.h"

#include <QPainter>
#include <QPainterPath>

#include <algorithm>

namespace {
constexpr int kHeight = 56;
// Display range; actual signal + noise floor levels vary a lot by sound
// card and mic gain, so these are just reasonable defaults for "something
// is there" visibility rather than a calibrated reading.
constexpr float kMinDb = -90.0f;
constexpr float kMaxDb = -20.0f;
constexpr int kDefaultToneHz = 1500; // matches Jtty::defaultToneHz
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
    m_magnitudesDb = magnitudesDb;
    update();
}

void SpectrumWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor(18, 22, 30));

    const int w = width();
    const int h = height();

    // Marker at the default JTTY tone, so it's obvious where to look.
    if (m_lowHz <= kDefaultToneHz && kDefaultToneHz <= m_highHz) {
        const float x = w * float(kDefaultToneHz - m_lowHz) / float(m_highHz - m_lowHz);
        painter.setPen(QPen(QColor(255, 255, 255, 50), 1, Qt::DashLine));
        painter.drawLine(QPointF(x, 0), QPointF(x, h));
    }

    const int n = m_magnitudesDb.size();
    if (n < 2)
        return;

    QPolygonF line;
    line.reserve(n);
    for (int i = 0; i < n; ++i) {
        const float normalized = (m_magnitudesDb[i] - kMinDb) / (kMaxDb - kMinDb);
        const float clamped = std::clamp(normalized, 0.0f, 1.0f);
        const float x = w * float(i) / float(n - 1);
        const float y = h - clamped * h;
        line << QPointF(x, y);
    }

    QPainterPath fillPath;
    fillPath.moveTo(line.first().x(), h);
    for (const QPointF &point : line)
        fillPath.lineTo(point);
    fillPath.lineTo(line.last().x(), h);
    fillPath.closeSubpath();

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(50, 200, 140, 130));
    painter.drawPath(fillPath);

    QPen linePen(QColor(110, 240, 180));
    linePen.setWidthF(1.5);
    painter.setPen(linePen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPolyline(line);
}
