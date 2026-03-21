#include "signalgenerator.h"

// ============================================================
// SignalGenerator (UI thread proxy)
// ============================================================

SignalGenerator::SignalGenerator(QObject *parent) : QObject(parent) {
    m_worker = new SignalWorker();
    m_worker->moveToThread(&m_workerThread);

    connect(m_worker, &SignalWorker::dataReady,
            this, &SignalGenerator::handleDataReady, Qt::QueuedConnection);
    connect(m_worker, &SignalWorker::triggerStatusChanged,
            this, &SignalGenerator::handleTriggerStatus, Qt::QueuedConnection);

    QTimer *timer = new QTimer();
    timer->setInterval(16); // ~60 FPS
    timer->moveToThread(&m_workerThread);

    connect(timer, &QTimer::timeout, m_worker, &SignalWorker::generateData);
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
    if (channel == 0 && m_series1) {
        m_series1->replace(points);
    } else if (channel == 1 && m_series2) {
        m_series2->replace(points);
    }
}

void SignalGenerator::handleTriggerStatus(const QString &status) {
    if (m_triggerStatus != status) {
        m_triggerStatus = status;
        emit triggerStatusChanged(status);
    }
}

// --- Property setters (proxy to worker thread) ---

void SignalGenerator::setTriggerLevel(double level) {
    if (!qFuzzyCompare(m_triggerLevel, level)) {
        m_triggerLevel = level;
        QMetaObject::invokeMethod(m_worker, "setTriggerLevel",
                                  Qt::QueuedConnection, Q_ARG(double, level));
        emit triggerLevelChanged();
    }
}

void SignalGenerator::setTriggerSource(int channel) {
    if (m_triggerSource != channel) {
        m_triggerSource = channel;
        QMetaObject::invokeMethod(m_worker, "setTriggerSource",
                                  Qt::QueuedConnection, Q_ARG(int, channel));
        emit triggerSourceChanged();
    }
}

void SignalGenerator::setTriggerEdge(int edge) {
    if (m_triggerEdge != edge) {
        m_triggerEdge = edge;
        QMetaObject::invokeMethod(m_worker, "setTriggerEdge",
                                  Qt::QueuedConnection, Q_ARG(int, edge));
        emit triggerEdgeChanged();
    }
}

void SignalGenerator::setRunning(bool running) {
    if (m_running != running) {
        m_running = running;
        QMetaObject::invokeMethod(m_worker, "setRunning",
                                  Qt::QueuedConnection, Q_ARG(bool, running));
        emit runningChanged();
    }
}

void SignalGenerator::setTriggerHoldoff(double s) {
    QMetaObject::invokeMethod(m_worker, "setTriggerHoldoff",
                              Qt::QueuedConnection, Q_ARG(double, s));
}

void SignalGenerator::setPreTriggerPercent(double p) {
    QMetaObject::invokeMethod(m_worker, "setPreTriggerPercent",
                              Qt::QueuedConnection, Q_ARG(double, p));
}

void SignalGenerator::forceTrigger() {
    QMetaObject::invokeMethod(m_worker, "forceTrigger", Qt::QueuedConnection);
}

void SignalGenerator::setAcquisitionMode(int mode) {
    QMetaObject::invokeMethod(m_worker, "setAcquisitionMode",
                              Qt::QueuedConnection, Q_ARG(int, mode));
}

void SignalGenerator::setSampleRate(double rate) {
    QMetaObject::invokeMethod(m_worker, "setSampleRate",
                              Qt::QueuedConnection, Q_ARG(double, rate));
}

void SignalGenerator::setTimebaseSeconds(double s) {
    QMetaObject::invokeMethod(m_worker, "setTimebaseSeconds",
                              Qt::QueuedConnection, Q_ARG(double, s));
}

void SignalGenerator::setHorizontalPosition(double divs) {
    QMetaObject::invokeMethod(m_worker, "setHorizontalPosition",
                              Qt::QueuedConnection, Q_ARG(double, divs));
}

void SignalGenerator::setCh1Offset(double v) {
    QMetaObject::invokeMethod(m_worker, "setCh1Offset",
                              Qt::QueuedConnection, Q_ARG(double, v));
}

void SignalGenerator::setCh2Offset(double v) {
    QMetaObject::invokeMethod(m_worker, "setCh2Offset",
                              Qt::QueuedConnection, Q_ARG(double, v));
}

// --- Per-channel simulation config proxies ---

void SignalGenerator::setCh1Waveform(int w) {
    QMetaObject::invokeMethod(m_worker, "setCh1Waveform",
                              Qt::QueuedConnection, Q_ARG(int, w));
}
void SignalGenerator::setCh1Amplitude(double v) {
    QMetaObject::invokeMethod(m_worker, "setCh1Amplitude",
                              Qt::QueuedConnection, Q_ARG(double, v));
}
void SignalGenerator::setCh1Frequency(double v) {
    QMetaObject::invokeMethod(m_worker, "setCh1Frequency",
                              Qt::QueuedConnection, Q_ARG(double, v));
}
void SignalGenerator::setCh1Phase(double v) {
    QMetaObject::invokeMethod(m_worker, "setCh1Phase",
                              Qt::QueuedConnection, Q_ARG(double, v));
}
void SignalGenerator::setCh1DcOffset(double v) {
    QMetaObject::invokeMethod(m_worker, "setCh1DcOffset",
                              Qt::QueuedConnection, Q_ARG(double, v));
}
void SignalGenerator::setCh1DutyCycle(double v) {
    QMetaObject::invokeMethod(m_worker, "setCh1DutyCycle",
                              Qt::QueuedConnection, Q_ARG(double, v));
}
void SignalGenerator::setCh1Noise(double v) {
    QMetaObject::invokeMethod(m_worker, "setCh1Noise",
                              Qt::QueuedConnection, Q_ARG(double, v));
}

void SignalGenerator::setCh2Waveform(int w) {
    QMetaObject::invokeMethod(m_worker, "setCh2Waveform",
                              Qt::QueuedConnection, Q_ARG(int, w));
}
void SignalGenerator::setCh2Amplitude(double v) {
    QMetaObject::invokeMethod(m_worker, "setCh2Amplitude",
                              Qt::QueuedConnection, Q_ARG(double, v));
}
void SignalGenerator::setCh2Frequency(double v) {
    QMetaObject::invokeMethod(m_worker, "setCh2Frequency",
                              Qt::QueuedConnection, Q_ARG(double, v));
}
void SignalGenerator::setCh2Phase(double v) {
    QMetaObject::invokeMethod(m_worker, "setCh2Phase",
                              Qt::QueuedConnection, Q_ARG(double, v));
}
void SignalGenerator::setCh2DcOffset(double v) {
    QMetaObject::invokeMethod(m_worker, "setCh2DcOffset",
                              Qt::QueuedConnection, Q_ARG(double, v));
}
void SignalGenerator::setCh2DutyCycle(double v) {
    QMetaObject::invokeMethod(m_worker, "setCh2DutyCycle",
                              Qt::QueuedConnection, Q_ARG(double, v));
}
void SignalGenerator::setCh2Noise(double v) {
    QMetaObject::invokeMethod(m_worker, "setCh2Noise",
                              Qt::QueuedConnection, Q_ARG(double, v));
}

// ============================================================
// SignalWorker (background thread logic)
// ============================================================

double SignalWorker::gaussianRandom() {
    return m_normalDist(m_rng);
}

void SignalWorker::setAcquisitionMode(int mode) {
    m_acquisitionMode = mode;
    m_singleShotFired = false;
    m_forceTrigger = false;
}

double SignalWorker::generateSample(const ChannelSimConfig &cfg, double t) {
    double value = 0.0;
    double phase = 2.0 * M_PI * cfg.frequency * t + cfg.phase;

    switch (cfg.waveform) {
    case WaveformType::Sine:
        value = cfg.amplitude * std::sin(phase);
        break;
    case WaveformType::Square: {
        double p = std::fmod(phase, 2.0 * M_PI);
        if (p < 0.0) p += 2.0 * M_PI;
        value = cfg.amplitude * (p < 2.0 * M_PI * cfg.dutyCycle ? 1.0 : -1.0);
        break;
    }
    case WaveformType::Triangle: {
        double p = std::fmod(phase / (2.0 * M_PI), 1.0);
        if (p < 0.0) p += 1.0;
        value = cfg.amplitude * (p < 0.5 ? 4.0 * p - 1.0 : 3.0 - 4.0 * p);
        break;
    }
    case WaveformType::Sawtooth: {
        double p = std::fmod(phase / (2.0 * M_PI), 1.0);
        if (p < 0.0) p += 1.0;
        value = cfg.amplitude * (2.0 * p - 1.0);
        break;
    }
    case WaveformType::Pulse: {
        double p = std::fmod(phase, 2.0 * M_PI);
        if (p < 0.0) p += 2.0 * M_PI;
        value = cfg.amplitude * (p < 2.0 * M_PI * cfg.dutyCycle ? 1.0 : 0.0);
        break;
    }
    case WaveformType::DC:
        value = cfg.amplitude;
        break;
    case WaveformType::Noise:
        // Pure noise, base value is zero
        break;
    }

    value += cfg.dcOffset;

    if (cfg.noise > 0.0) {
        value += cfg.noise * gaussianRandom();
    }

    return value;
}

int SignalWorker::findTriggerPoint(const std::vector<double> &samples, int numSamples) {
    const double preTriggerFrac = m_preTriggerPercent / 100.0;
    const int targetDisplay = 400;
    const int preTriggerSamples = static_cast<int>(targetDisplay * preTriggerFrac);
    const int searchStart = preTriggerSamples;
    const int searchEnd = numSamples - targetDisplay;
    const int holdoffSamples = static_cast<int>(m_triggerHoldoff * m_sampleRate);

    if (searchEnd <= searchStart) return -1;

    for (int i = searchStart; i < searchEnd; ++i) {
        if (holdoffSamples > 0 && i < holdoffSamples) continue;

        double prev = samples[i - 1];
        double curr = samples[i];

        bool triggered = false;
        if (m_triggerEdge == 0) {
            triggered = (prev < m_triggerLevel && curr >= m_triggerLevel);
        } else {
            triggered = (prev > m_triggerLevel && curr <= m_triggerLevel);
        }

        if (triggered) {
            return i - preTriggerSamples;
        }
    }

    return -1; // No trigger found
}

void SignalWorker::generateData() {
    if (!m_running) return;
    if (!m_ch1Active && !m_ch2Active) return;

    // Single shot already fired
    if (m_acquisitionMode == 2 && m_singleShotFired && !m_forceTrigger) {
        return;
    }

    const int targetPoints = 400;
    const double totalTime = m_timebaseSeconds * 10.0;
    const double dt = 1.0 / m_sampleRate;

    // Number of internal samples — cap to avoid excessive computation
    int numSamples = static_cast<int>(totalTime * m_sampleRate);
    if (numSamples < targetPoints) numSamples = targetPoints;
    if (numSamples > 100000) numSamples = 100000;

    // Compute actual dt based on capped sample count
    const double actualDt = totalTime / numSamples;

    // Generate trigger source samples for trigger detection
    const ChannelSimConfig &trigCfg = (m_triggerSource == 0) ? m_ch1Config : m_ch2Config;
    double trigOffset = (m_triggerSource == 0) ? m_ch1Offset : m_ch2Offset;

    std::vector<double> trigSamples(numSamples);
    for (int i = 0; i < numSamples; ++i) {
        double t = m_timeOffset + i * actualDt;
        trigSamples[i] = generateSample(trigCfg, t) + trigOffset;
    }

    // Find trigger point
    int triggerIdx = findTriggerPoint(trigSamples, numSamples);
    bool triggered = (triggerIdx >= 0) || m_forceTrigger;

    // Handle acquisition modes
    QString status;
    if (m_acquisitionMode == 1 && !triggered) {
        // Normal mode: no trigger found, don't update
        status = "Waiting";
        emit triggerStatusChanged(status);
        return;
    } else if (m_acquisitionMode == 2) {
        if (!triggered) {
            status = "Waiting";
            emit triggerStatusChanged(status);
            return;
        }
        if (m_singleShotFired && !m_forceTrigger) {
            status = "Stopped";
            emit triggerStatusChanged(status);
            return;
        }
        m_singleShotFired = true;
        status = "Triggered";
    } else {
        // Auto mode
        status = triggered ? "Triggered" : "Auto";
    }

    m_forceTrigger = false;
    emit triggerStatusChanged(status);

    // If no trigger found in Auto mode, free-run from start
    if (triggerIdx < 0) triggerIdx = 0;

    // Apply horizontal position shift (in divisions)
    int hShiftSamples = static_cast<int>(m_horizontalPosition * (totalTime / 10.0) / actualDt);
    triggerIdx += hShiftSamples;
    if (triggerIdx < 0) triggerIdx = 0;
    if (triggerIdx + targetPoints > numSamples) triggerIdx = numSamples - targetPoints;
    if (triggerIdx < 0) triggerIdx = 0;

    // Compute decimation: how many internal samples map to each display point
    int windowSamples = numSamples - triggerIdx;
    if (windowSamples < targetPoints) windowSamples = targetPoints;
    int bucketSize = windowSamples / targetPoints;
    if (bucketSize < 1) bucketSize = 1;

    // Generate display points with min/max decimation
    QList<QPointF> points1, points2;
    if (m_ch1Active) points1.reserve(targetPoints);
    if (m_ch2Active) points2.reserve(targetPoints);

    double timePerPoint = totalTime / targetPoints;

    for (int b = 0; b < targetPoints; ++b) {
        double displayTime = b * timePerPoint;

        if (m_ch1Active) {
            double maxV = -1e9, minV = 1e9;
            for (int j = 0; j < bucketSize; ++j) {
                int idx = triggerIdx + b * bucketSize + j;
                if (idx < 0 || idx >= numSamples) continue;
                double t = m_timeOffset + idx * actualDt;
                double v = generateSample(m_ch1Config, t) + m_ch1Offset;
                if (v > maxV) maxV = v;
                if (v < minV) minV = v;
            }
            if (maxV < -1e8) maxV = 0; // safety for empty bucket
            if (minV > 1e8) minV = 0;
            points1.append(QPointF(displayTime, (b % 2 == 0) ? maxV : minV));
        }

        if (m_ch2Active) {
            double maxV = -1e9, minV = 1e9;
            for (int j = 0; j < bucketSize; ++j) {
                int idx = triggerIdx + b * bucketSize + j;
                if (idx < 0 || idx >= numSamples) continue;
                double t = m_timeOffset + idx * actualDt;
                double v = generateSample(m_ch2Config, t) + m_ch2Offset;
                if (v > maxV) maxV = v;
                if (v < minV) minV = v;
            }
            if (maxV < -1e8) maxV = 0;
            if (minV > 1e8) minV = 0;
            points2.append(QPointF(displayTime, (b % 2 == 0) ? maxV : minV));
        }
    }

    if (m_ch1Active) emit dataReady(0, points1);
    if (m_ch2Active) emit dataReady(1, points2);

    // Advance time for continuous waveform motion
    m_timeOffset += totalTime * 0.02; // Advance by 2% of window per frame
}
