#include "signalgenerator.h"

SignalGenerator::SignalGenerator(QObject *parent) : QObject(parent) {
    m_worker = new SignalWorker();
    m_worker->moveToThread(&m_workerThread);

    // Connect worker's data signal to our handler (queued connection for thread safety)
    connect(m_worker, &SignalWorker::dataReady, this, &SignalGenerator::handleDataReady, Qt::QueuedConnection);

    // Setup the timer to live on the worker thread
    QTimer *timer = new QTimer();
    timer->setInterval(16); // ~60 FPS
    timer->moveToThread(&m_workerThread);

    // Connect timer to worker's processing function
    connect(timer, &QTimer::timeout, m_worker, &SignalWorker::generateData);

    // Start/Stop logic
    connect(&m_workerThread, &QThread::started, timer, [=](){ timer->start(); });
    connect(&m_workerThread, &QThread::finished, timer, &QObject::deleteLater);
    connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);

    m_workerThread.start();
}

SignalGenerator::~SignalGenerator() {
    m_workerThread.quit();
    m_workerThread.wait();
}

void SignalGenerator::registerSeries(int channel, QXYSeries *series) {
    if (channel == 0) {
        m_series1 = series;
        QMetaObject::invokeMethod(m_worker, "setChannels",
                                  Qt::QueuedConnection,
                                  Q_ARG(bool, true),
                                  Q_ARG(bool, m_series2 != nullptr));
    } else if (channel == 1) {
        m_series2 = series;
        QMetaObject::invokeMethod(m_worker, "setChannels",
                                  Qt::QueuedConnection,
                                  Q_ARG(bool, m_series1 != nullptr),
                                  Q_ARG(bool, true));
    }
}

void SignalGenerator::handleDataReady(int channel, const QList<QPointF> &points) {
    // Update series directly in UI thread - this is thread-safe
    if (channel == 0 && m_series1) {
        m_series1->replace(points);
    } else if (channel == 1 && m_series2) {
        m_series2->replace(points);
    }
}

void SignalGenerator::setTriggerLevel(double level) {
    // Push the new trigger level to the worker thread
    QMetaObject::invokeMethod(m_worker, "setTriggerLevel",
                              Qt::QueuedConnection,
                              Q_ARG(double, level));
}

// --- Worker Logic ---
void SignalWorker::generateData() {
    if (!m_ch1Active && !m_ch2Active) return;

    const int targetPoints = 400;
    const int internalBuffer = 10000;
    const int bucketSize = 20;

    // 1. Trigger Search
    int triggerIdx = 0;
    bool triggered = false;
    for (int i = 1; i < (internalBuffer - targetPoints); ++i) {
        double prev = std::sin(0.05 * (i - 1) + m_index);
        double curr = std::sin(0.05 * i + m_index);
        if (prev < m_triggerLevel && curr >= m_triggerLevel) {
            triggerIdx = i;
            triggered = true;
            break;
        }
    }
    if (!triggered) triggerIdx = 0;

    // 2. Data Generation with Decimation
    QList<QPointF> points1, points2;
    if (m_ch1Active) points1.reserve(targetPoints);
    if (m_ch2Active) points2.reserve(targetPoints);

    for (int b = 0; b < targetPoints; ++b) {
        if (m_ch1Active) {
            double maxV = -100.0, minV = 100.0;
            for (int j = 0; j < bucketSize; ++j) {
                double v = std::sin(0.05 * (triggerIdx + b * bucketSize + j) + m_index);
                if (v > maxV) maxV = v;
                if (v < minV) minV = v;
            }
            points1.append(QPointF(b, (b % 2 == 0) ? maxV : minV));
        }
        if (m_ch2Active) {
            double maxV = -100.0, minV = 100.0;
            for (int j = 0; j < bucketSize; ++j) {
                double v = std::cos(0.03 * (triggerIdx + b * bucketSize + j) + (m_index * 0.5));
                if (v > maxV) maxV = v;
                if (v < minV) minV = v;
            }
            points2.append(QPointF(b, (b % 2 == 0) ? maxV : minV));
        }
    }

    if (m_ch1Active) emit dataReady(0, points1);
    if (m_ch2Active) emit dataReady(1, points2);

    m_index += 0.1;
}
