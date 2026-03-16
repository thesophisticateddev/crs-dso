#include "ScopeController.h"
#include "signalgenerator.h"
#include "AcquisitionEngine.h"
#include "WaveformBuffer.h"
#include "transport/ITransport.h"
#include "transport/MockTransport.h"
#ifdef HAS_LIBUSB
#include "transport/UsbTransport.h"
#endif
#include <QCoreApplication>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QPointF>
#include <cmath>

// --- Fix #2: Standard 1-2-5 timebase sequence (in seconds) ---
const QVector<double> ScopeController::s_timebaseSteps = {
    // ns range
    5e-9, 10e-9, 20e-9, 50e-9, 100e-9, 200e-9, 500e-9,
    // us range
    1e-6, 2e-6, 5e-6, 10e-6, 20e-6, 50e-6, 100e-6, 200e-6, 500e-6,
    // ms range
    1e-3, 2e-3, 5e-3, 10e-3, 20e-3, 50e-3, 100e-3, 200e-3, 500e-3,
    // s range
    1.0, 2.0, 5.0, 10.0
};

// --- Fix #2: Standard 1-2-5 volts/div sequence ---
const QVector<double> ScopeController::s_voltsSteps = {
    1e-3, 2e-3, 5e-3,       // mV range
    10e-3, 20e-3, 50e-3,
    100e-3, 200e-3, 500e-3,
    1.0, 2.0, 5.0, 10.0     // V range
};

ScopeController::ScopeController(QObject* parent)
    : QObject(parent)
{
    // Start in test mode with signal generator
    setupSignalGenerator();
}

ScopeController::~ScopeController() {
    teardownSignalGenerator();
    if (m_engine) {
        m_engine->requestStop();
        m_engine->wait(3000);
    }
}

// --- Series registration (Fix #4: direct series path) ---

void ScopeController::registerSeries(int channel, QXYSeries* series) {
    if (channel == 0) {
        m_series1 = series;
    } else if (channel == 1) {
        m_series2 = series;
    }

    // Forward to signal generator if in test mode
    if (m_testMode && m_signalGen) {
        m_signalGen->registerSeries(channel, series);
    }
}

// --- Fix #5: Async device scanning ---

void ScopeController::scanDevices() {
    m_statusText = "Scanning for devices...";
    emit statusChanged();

    auto future = QtConcurrent::run([]() -> std::vector<DeviceInfo> {
#ifdef HAS_LIBUSB
        UsbTransport usb;
        return usb.enumerate();
#else
        MockTransport mock;
        return mock.enumerate();
#endif
    });

    auto* watcher = new QFutureWatcher<std::vector<DeviceInfo>>(this);
    connect(watcher, &QFutureWatcher<std::vector<DeviceInfo>>::finished,
            this, [this, watcher]() {
        m_discoveredDevices = watcher->result();
        if (m_discoveredDevices.empty()) {
            m_statusText = "No devices found";
        } else {
            m_statusText = QString("Found %1 device(s)").arg(m_discoveredDevices.size());
            connectToFirstDevice();
        }
        emit statusChanged();
        watcher->deleteLater();
    });
    watcher->setFuture(future);
}

void ScopeController::connectToFirstDevice() {
    if (m_discoveredDevices.empty()) return;

    bool useMock = QCoreApplication::arguments().contains("--demoMode");
    std::unique_ptr<ITransport> transport;
    if (useMock) {
        transport = std::make_unique<MockTransport>();
    } else {
#ifdef HAS_LIBUSB
        transport = std::make_unique<UsbTransport>();
#else
        transport = std::make_unique<MockTransport>();
#endif
    }

    m_engine = std::make_unique<AcquisitionEngine>(std::move(transport));

    connect(m_engine.get(), &AcquisitionEngine::waveformReady,
            this, &ScopeController::updateSeriesFromWaveform, Qt::QueuedConnection);
    connect(m_engine.get(), &AcquisitionEngine::stateChanged,
            this, [this](EngineState state) {
        bool wasRunning = m_running;
        m_running = (state == EngineState::Armed || state == EngineState::Acquiring);
        if (wasRunning != m_running) emit runningChanged();

        switch (state) {
        case EngineState::Idle:      m_statusText = "Connected - Idle"; break;
        case EngineState::Armed:     m_statusText = "Armed"; break;
        case EngineState::Acquiring: m_statusText = "Acquiring"; break;
        case EngineState::Stopped:   m_statusText = "Stopped"; break;
        case EngineState::Error:     m_statusText = "Error"; break;
        default: break;
        }
        emit statusChanged();
    }, Qt::QueuedConnection);
    connect(m_engine.get(), &AcquisitionEngine::errorOccurred,
            this, [this](const QString& msg) {
        m_statusText = "Error: " + msg;
        emit statusChanged();
        emit errorOccurred(msg);
    }, Qt::QueuedConnection);
    connect(m_engine.get(), &AcquisitionEngine::connectionChanged,
            this, [this](bool connected) {
        if (connected) {
            m_statusText = "Connected";
        } else if (!m_testMode) {
            m_statusText = "Disconnected";
        }
        emit statusChanged();
    }, Qt::QueuedConnection);
    connect(m_engine.get(), &AcquisitionEngine::deviceIdentified,
            this, [this](const QString& model, const QString& /*firmware*/) {
        m_statusText = "Connected: " + model;
        emit statusChanged();
    }, Qt::QueuedConnection);

    m_engine->connectDevice(m_discoveredDevices[0]);
    m_statusText = "Connecting...";
    emit statusChanged();
}

// --- Step list accessors ---

QVariantList ScopeController::timebaseSteps() const {
    QVariantList list;
    list.reserve(s_timebaseSteps.size());
    for (double v : s_timebaseSteps) list.append(v);
    return list;
}

QVariantList ScopeController::voltsPerDivSteps() const {
    QVariantList list;
    list.reserve(s_voltsSteps.size());
    for (double v : s_voltsSteps) list.append(v);
    return list;
}

// --- Fix #2: Timebase label formatting ---

QString ScopeController::timebaseLabel() const {
    double val = s_timebaseSteps[m_timebaseIndex];
    if (val < 1e-6)      return QString::number(val * 1e9, 'f', 0) + " ns/div";
    else if (val < 1e-3) return QString::number(val * 1e6, 'f', 0) + " us/div";
    else if (val < 1.0)  return QString::number(val * 1e3, 'f', 0) + " ms/div";
    else                 return QString::number(val, 'f', 0) + " s/div";
}

// --- Getters ---

bool ScopeController::testMode() const { return m_testMode; }
bool ScopeController::running() const { return m_running; }
QString ScopeController::statusText() const { return m_statusText; }
int ScopeController::triggerSource() const { return m_triggerSource; }
int ScopeController::triggerEdge() const { return m_triggerEdge; }
double ScopeController::triggerLevel() const { return m_triggerLevel; }
int ScopeController::timebaseIndex() const { return m_timebaseIndex; }
bool ScopeController::ch1Enabled() const { return m_ch1Enabled; }
int ScopeController::ch1VoltsIndex() const { return m_ch1VoltsIndex; }
int ScopeController::ch1Coupling() const { return m_ch1Coupling; }
double ScopeController::ch1Offset() const { return m_ch1Offset; }
bool ScopeController::ch2Enabled() const { return m_ch2Enabled; }
int ScopeController::ch2VoltsIndex() const { return m_ch2VoltsIndex; }
int ScopeController::ch2Coupling() const { return m_ch2Coupling; }
double ScopeController::ch2Offset() const { return m_ch2Offset; }
int ScopeController::acquisitionMode() const { return m_acquisitionMode; }

// --- Setters ---

void ScopeController::setTestMode(bool v) {
    if (m_testMode == v) return;
    m_testMode = v;

    if (v) {
        // Switch to test mode
        if (m_engine) {
            m_engine->stopAcquisition();
            m_engine->requestStop();
            m_engine->wait(3000);
            m_engine.reset();
        }
        setupSignalGenerator();
        m_running = true;
        m_statusText = "Emulated oscilloscope";
    } else {
        // Switch to device mode
        teardownSignalGenerator();
        m_running = false;
        m_statusText = "Scanning for devices...";
        scanDevices();
    }

    emit modeChanged();
    emit runningChanged();
    emit statusChanged();
}

void ScopeController::setRunning(bool v) {
    if (m_running == v) return;

    if (m_testMode) {
        m_running = v;
        if (m_signalGen) m_signalGen->setRunning(v);
    } else {
        if (v) {
            if (m_engine) m_engine->startAcquisition(static_cast<AcquisitionMode>(m_acquisitionMode));
            m_running = true;
        } else {
            if (m_engine) m_engine->stopAcquisition();
            m_running = false;
        }
    }
    emit runningChanged();
}

void ScopeController::setTriggerSource(int v) {
    if (m_triggerSource == v) return;
    m_triggerSource = v;
    if (m_testMode && m_signalGen) {
        m_signalGen->setTriggerSource(v);
    } else if (m_engine) {
        m_engine->setTrigger(static_cast<uint8_t>(v),
                             static_cast<uint8_t>(m_triggerEdge),
                             static_cast<int16_t>(m_triggerLevel * 1000));
    }
    emit triggerChanged();
}

void ScopeController::setTriggerEdge(int v) {
    if (m_triggerEdge == v) return;
    m_triggerEdge = v;
    if (m_testMode && m_signalGen) {
        m_signalGen->setTriggerEdge(v);
    } else if (m_engine) {
        m_engine->setTrigger(static_cast<uint8_t>(m_triggerSource),
                             static_cast<uint8_t>(v),
                             static_cast<int16_t>(m_triggerLevel * 1000));
    }
    emit triggerChanged();
}

void ScopeController::setTriggerLevel(double v) {
    if (qFuzzyCompare(m_triggerLevel, v)) return;
    m_triggerLevel = v;
    if (m_testMode && m_signalGen) {
        m_signalGen->setTriggerLevel(v);
    } else if (m_engine) {
        m_engine->setTrigger(static_cast<uint8_t>(m_triggerSource),
                             static_cast<uint8_t>(m_triggerEdge),
                             static_cast<int16_t>(v * 1000));
    }
    emit triggerChanged();
}

void ScopeController::setTimebaseIndex(int v) {
    if (v < 0 || v >= s_timebaseSteps.size()) return;
    if (m_timebaseIndex == v) return;
    m_timebaseIndex = v;
    if (!m_testMode && m_engine) {
        double secs = s_timebaseSteps[v];
        uint32_t nsPerDiv = static_cast<uint32_t>(secs * 1e9);
        m_engine->setTimebase(nsPerDiv);
    }
    emit timebaseChanged();
}

void ScopeController::setCh1Enabled(bool v) {
    if (m_ch1Enabled == v) return;
    m_ch1Enabled = v;
    if (!m_testMode && m_engine) m_engine->setChannelEnabled(0, v);
    emit channelChanged();
}

void ScopeController::setCh1VoltsIndex(int v) {
    if (v < 0 || v >= s_voltsSteps.size()) return;
    if (m_ch1VoltsIndex == v) return;
    m_ch1VoltsIndex = v;
    if (!m_testMode && m_engine)
        m_engine->setVoltageRange(0, static_cast<uint8_t>(v));
    emit channelChanged();
}

void ScopeController::setCh1Coupling(int v) {
    if (m_ch1Coupling == v) return;
    m_ch1Coupling = v;
    if (!m_testMode && m_engine)
        m_engine->setCoupling(0, static_cast<uint8_t>(v));
    emit channelChanged();
}

void ScopeController::setCh1Offset(double v) {
    if (qFuzzyCompare(m_ch1Offset, v)) return;
    m_ch1Offset = v;
    if (!m_testMode && m_engine)
        m_engine->setChannelOffset(0, static_cast<int16_t>(v * 1000));
    emit channelChanged();
}

void ScopeController::setCh2Enabled(bool v) {
    if (m_ch2Enabled == v) return;
    m_ch2Enabled = v;
    if (!m_testMode && m_engine) m_engine->setChannelEnabled(1, v);
    emit channelChanged();
}

void ScopeController::setCh2VoltsIndex(int v) {
    if (v < 0 || v >= s_voltsSteps.size()) return;
    if (m_ch2VoltsIndex == v) return;
    m_ch2VoltsIndex = v;
    if (!m_testMode && m_engine)
        m_engine->setVoltageRange(1, static_cast<uint8_t>(v));
    emit channelChanged();
}

void ScopeController::setCh2Coupling(int v) {
    if (m_ch2Coupling == v) return;
    m_ch2Coupling = v;
    if (!m_testMode && m_engine)
        m_engine->setCoupling(1, static_cast<uint8_t>(v));
    emit channelChanged();
}

void ScopeController::setCh2Offset(double v) {
    if (qFuzzyCompare(m_ch2Offset, v)) return;
    m_ch2Offset = v;
    if (!m_testMode && m_engine)
        m_engine->setChannelOffset(1, static_cast<int16_t>(v * 1000));
    emit channelChanged();
}

void ScopeController::setAcquisitionMode(int v) {
    if (m_acquisitionMode == v) return;
    m_acquisitionMode = v;
    emit acquisitionModeChanged();
}

// --- Fix #4: Direct series update from waveform buffer ---

void ScopeController::updateSeriesFromWaveform(std::shared_ptr<WaveformBuffer> buf) {
    if (!buf) return;

    double timePerDiv = s_timebaseSteps[m_timebaseIndex];
    double totalTime = timePerDiv * 10.0;

    if (m_series1 && buf->channels.size() > 0) {
        const auto& raw = buf->channels[0];
        QList<QPointF> points;
        points.reserve(static_cast<int>(raw.size()));
        double dt = totalTime / raw.size();
        for (size_t i = 0; i < raw.size(); i++) {
            points.append(QPointF(i * dt, buf->toVoltage(raw[i])));
        }
        m_series1->replace(points);
    }

    if (m_series2 && buf->channels.size() > 1) {
        const auto& raw = buf->channels[1];
        QList<QPointF> points;
        points.reserve(static_cast<int>(raw.size()));
        double dt = totalTime / raw.size();
        for (size_t i = 0; i < raw.size(); i++) {
            points.append(QPointF(i * dt, buf->toVoltage(raw[i])));
        }
        m_series2->replace(points);
    }
}

// --- Signal generator lifecycle ---

void ScopeController::setupSignalGenerator() {
    m_signalGen = std::make_unique<SignalGenerator>();
    m_signalGen->setRunning(true);
    m_signalGen->setTriggerLevel(m_triggerLevel);
    m_signalGen->setTriggerSource(m_triggerSource);
    m_signalGen->setTriggerEdge(m_triggerEdge);

    // Re-register series if already set
    if (m_series1) m_signalGen->registerSeries(0, m_series1);
    if (m_series2) m_signalGen->registerSeries(1, m_series2);

    m_running = true;
}

void ScopeController::teardownSignalGenerator() {
    if (m_signalGen) {
        m_signalGen->setRunning(false);
        m_signalGen.reset();
    }
}
