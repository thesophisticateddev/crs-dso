#include "signalgenerator.h"
#include <QPointF>
#include <QList>

SignalGenerator::SignalGenerator(QObject *parent) : QObject(parent) {
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &SignalGenerator::generateData);
    m_timer->start(16);
}

void SignalGenerator::updateSeries(int channel, QAbstractSeries *series) {
    if (channel == 0)
        m_series1 = static_cast<QXYSeries *>(series);
    else if (channel == 1)
        m_series2 = static_cast<QXYSeries *>(series);
}

void SignalGenerator::generateData() {
    QList<QPointF> points1;
    QList<QPointF> points2;

    points1.reserve(m_maxPoints);
    points2.reserve(m_maxPoints);

    for (int i = 0; i < m_maxPoints; ++i) {
        double x = i;

        // Channel 1: Sine Wave
        if (m_series1) {
            double y1 = std::sin(0.05 * i + m_index);
            points1.append(QPointF(x, y1));
        }

        // Channel 2: Cosine Wave with different frequency
        if (m_series2) {
            double y2 = std::cos(0.03 * i + (m_index * 0.5));
            points2.append(QPointF(x, y2));
        }
    }

    if (m_series1) m_series1->replace(points1);
    if (m_series2) m_series2->replace(points2);

    m_index += 0.1;
}
