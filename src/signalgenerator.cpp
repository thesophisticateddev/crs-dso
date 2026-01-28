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
    if (m_triggerLevel != level) {
        m_triggerLevel = level;
        QMetaObject::invokeMethod(m_worker, "setTriggerLevel",
                                  Qt::QueuedConnection,
                                  Q_ARG(double, level));
        emit triggerLevelChanged();
    }
}

void SignalGenerator::setTriggerSource(int channel) {
    if (m_triggerSource != channel) {
        m_triggerSource = channel;
        QMetaObject::invokeMethod(m_worker, "setTriggerSource",
                                  Qt::QueuedConnection,
                                  Q_ARG(int, channel));
        emit triggerSourceChanged();
    }
}

void SignalGenerator::setTriggerEdge(int edge) {
    if (m_triggerEdge != edge) {
        m_triggerEdge = edge;
        QMetaObject::invokeMethod(m_worker, "setTriggerEdge",
                                  Qt::QueuedConnection,
                                  Q_ARG(int, edge));
        emit triggerEdgeChanged();
    }
}

void SignalGenerator::setRunning(bool running) {
    if (m_running != running) {
        m_running = running;
        QMetaObject::invokeMethod(m_worker, "setRunning",
                                  Qt::QueuedConnection,
                                  Q_ARG(bool, running));
        emit runningChanged();
    }
}

// --- Worker Logic ---

double SignalWorker::generateCh1Sample(int idx) {
    return std::sin(0.05 * idx + m_index);
}

double SignalWorker::generateCh2Sample(int idx) {
    return std::cos(0.03 * idx + (m_index * 0.5));
}

double SignalWorker::generateSample(int channel, int idx) {
    if (channel == 0) {
        return generateCh1Sample(idx);
    } else {
        return generateCh2Sample(idx);
    }
}

int SignalWorker::findTriggerPoint(int bufferSize, int displayPoints) {
    // Pre-trigger: show 25% of data before trigger point
    const int preTriggerPoints = displayPoints / 4;
    const int searchStart = preTriggerPoints;
    const int searchEnd = bufferSize - displayPoints;

    for (int i = searchStart; i < searchEnd; ++i) {
        double prev = generateSample(m_triggerSource, i - 1);
        double curr = generateSample(m_triggerSource, i);

        bool triggered = false;
        if (m_triggerEdge == 0) {
            // Rising edge: signal crosses from below to above trigger level
            triggered = (prev < m_triggerLevel && curr >= m_triggerLevel);
        } else {
            // Falling edge: signal crosses from above to below trigger level
            triggered = (prev > m_triggerLevel && curr <= m_triggerLevel);
        }

        if (triggered) {
            // Return index adjusted for pre-trigger display
            return i - preTriggerPoints;
        }
    }

    // No trigger found, return 0 (free-running mode)
    return 0;
}

void SignalWorker::generateData() {
    if (!m_running) return;
    if (!m_ch1Active && !m_ch2Active) return;

    const int targetPoints = 400;
    const int internalBuffer = 10000;
    const int bucketSize = 20;

    // Find trigger point using actual signal data
    int triggerIdx = findTriggerPoint(internalBuffer, targetPoints * bucketSize);

    // Data Generation with Decimation
    QList<QPointF> points1, points2;
    if (m_ch1Active) points1.reserve(targetPoints);
    if (m_ch2Active) points2.reserve(targetPoints);

    for (int b = 0; b < targetPoints; ++b) {
        if (m_ch1Active) {
            double maxV = -100.0, minV = 100.0;
            for (int j = 0; j < bucketSize; ++j) {
                double v = generateCh1Sample(triggerIdx + b * bucketSize + j);
                if (v > maxV) maxV = v;
                if (v < minV) minV = v;
            }
            points1.append(QPointF(b, (b % 2 == 0) ? maxV : minV));
        }
        if (m_ch2Active) {
            double maxV = -100.0, minV = 100.0;
            for (int j = 0; j < bucketSize; ++j) {
                double v = generateCh2Sample(triggerIdx + b * bucketSize + j);
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
