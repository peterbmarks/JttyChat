#pragma once

#include <QImage>
#include <QVector>
#include <QWidget>

// A small waterfall at the top of the window: each new magnitude spectrum
// of the received audio (over a fixed frequency band) becomes a new row,
// with older rows scrolling down over time. Fed by AudioSpectrum via
// setMagnitudesDb().
class SpectrumWidget : public QWidget
{
    Q_OBJECT

public:
    SpectrumWidget(int lowHz, int highHz, QWidget *parent = nullptr);

    QSize sizeHint() const override;

public slots:
    void setMagnitudesDb(const QVector<float> &magnitudesDb);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_lowHz;
    int m_highHz;
    QImage m_waterfall; // one column per bin, one row per update; newest row on top
};
