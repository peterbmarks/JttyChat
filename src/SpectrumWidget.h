#pragma once

#include <QWidget>
#include <QVector>

// A small strip at the top of the window showing the live magnitude
// spectrum of the received audio over a fixed frequency band. Fed by
// AudioSpectrum via setMagnitudesDb().
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
    QVector<float> m_magnitudesDb;
};
