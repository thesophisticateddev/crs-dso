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
    // Safety check: if no series are linked, don't waste CPU
    if (!m_series1 && !m_series2) return;

    const int internalBufferLimit = 10000; // Simulated high-speed raw buffer
    const int targetPoints = 400;         // UI display points
    const int bucketSize = 20;            // (internalBufferLimit / targetPoints) if not triggering

    // 1. Find the Trigger Point on Channel 1 (Rising Edge)
    int triggerStartIndex = 0;
    bool triggered = false;

    // We search the 'raw' data stream for the trigger event
    for (int i = 1; i < (internalBufferLimit - targetPoints); ++i) {
        double prevY = std::sin(0.05 * (i - 1) + m_index);
        double currY = std::sin(0.05 * i + m_index);

        if (prevY < m_triggerLevel && currY >= m_triggerLevel) {
            triggerStartIndex = i;
            triggered = true;
            break;
        }
    }

    // If we can't find a trigger (e.g. signal is too low),
    // real scopes usually 'Auto' trigger at 0 to keep the display moving.
    if (!triggered) triggerStartIndex = 0;

    QList<QPointF> points1;
    QList<QPointF> points2;
    points1.reserve(targetPoints);
    points2.reserve(targetPoints);

    // 2. Perform Min-Max Decimation for both channels starting from the trigger
    for (int b = 0; b < targetPoints; ++b) {
        double x = b;

        // CH1 Decimation
        if (m_series1) {
            double min1 = 100.0, max1 = -100.0;
            for (int j = 0; j < bucketSize; ++j) {
                double val = std::sin(0.05 * (triggerStartIndex + b * bucketSize + j) + m_index);
                if (val < min1) min1 = val;
                if (val > max1) max1 = val;
            }
            points1.append(QPointF(x, (b % 2 == 0) ? max1 : min1));
        }

        // CH2 Decimation (using the same trigger time-base)
        if (m_series2) {
            double min2 = 100.0, max2 = -100.0;
            for (int j = 0; j < bucketSize; ++j) {
                double val = std::cos(0.03 * (triggerStartIndex + b * bucketSize + j) + (m_index * 0.5));
                if (val < min2) min2 = val;
                if (val > max2) max2 = val;
            }
            points2.append(QPointF(x, (b % 2 == 0) ? max2 : min2));
        }
    }

    // 3. Update the UI
    if (m_series1) m_series1->replace(points1);
    if (m_series2) m_series2->replace(points2);

    m_index += 0.1; // Progress simulation time
}
