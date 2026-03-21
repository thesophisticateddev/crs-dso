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

// --- Standard 1-2-5 timebase sequence (in seconds) ---
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

// --- Standard 1-2-5 volts/div sequence ---
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

// --- Series registration ---

void ScopeController::registerSeries(int channel, QXYSeries* series) {
    if (channel == 0) {
        m_series1 = series;
    } else if (channel == 1) {
        m_series2 = series;
    }

    if (m_testMode && m_signalGen) {
        m_signalGen->registerSeries(channel, series);
    }
}

// --- Device scanning ---

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

QStringList ScopeController::waveformTypes() const {
    return {"Sine", "Square", "Triangle", "Sawtooth", "Pulse", "DC", "Noise"};
}

// --- Timebase label formatting ---

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
double ScopeController::triggerHoldoff() const { return m_triggerHoldoff; }
double ScopeController::preTriggerPercent() const { return m_preTriggerPercent; }
QString ScopeController::triggerStatus() const { return m_triggerStatus; }
int ScopeController::timebaseIndex() const { return m_timebaseIndex; }
double ScopeController::horizontalPosition() const { return m_horizontalPosition; }
bool ScopeController::ch1Enabled() const { return m_ch1Enabled; }
int ScopeController::ch1VoltsIndex() const { return m_ch1VoltsIndex; }
int ScopeController::ch1Coupling() const { return m_ch1Coupling; }
double ScopeController::ch1Offset() const { return m_ch1Offset; }
bool ScopeController::ch2Enabled() const { return m_ch2Enabled; }
int ScopeController::ch2VoltsIndex() const { return m_ch2VoltsIndex; }
int ScopeController::ch2Coupling() const { return m_ch2Coupling; }
double ScopeController::ch2Offset() const { return m_ch2Offset; }
int ScopeController::acquisitionMode() const { return m_acquisitionMode; }

// Simulation config getters
int ScopeController::ch1Waveform() const { return static_cast<int>(m_ch1SimConfig.waveform); }
double ScopeController::ch1Amplitude() const { return m_ch1SimConfig.amplitude; }
double ScopeController::ch1Frequency() const { return m_ch1SimConfig.frequency; }
double ScopeController::ch1Phase() const { return m_ch1SimConfig.phase; }
double ScopeController::ch1DcOffset() const { return m_ch1SimConfig.dcOffset; }
double ScopeController::ch1DutyCycle() const { return m_ch1SimConfig.dutyCycle; }
double ScopeController::ch1Noise() const { return m_ch1SimConfig.noise; }

int ScopeController::ch2Waveform() const { return static_cast<int>(m_ch2SimConfig.waveform); }
double ScopeController::ch2Amplitude() const { return m_ch2SimConfig.amplitude; }
double ScopeController::ch2Frequency() const { return m_ch2SimConfig.frequency; }
double ScopeController::ch2Phase() const { return m_ch2SimConfig.phase; }
double ScopeController::ch2DcOffset() const { return m_ch2SimConfig.dcOffset; }
double ScopeController::ch2DutyCycle() const { return m_ch2SimConfig.dutyCycle; }
double ScopeController::ch2Noise() const { return m_ch2SimConfig.noise; }

// Capability flags
bool ScopeController::supportsWaveformEditor() const { return m_testMode; }
bool ScopeController::supportsForceTrigger() const { return true; }

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

void ScopeController::setTriggerHoldoff(double v) {
    if (qFuzzyCompare(m_triggerHoldoff, v)) return;
    m_triggerHoldoff = v;
    if (m_testMode && m_signalGen) {
        m_signalGen->setTriggerHoldoff(v);
    }
    emit triggerChanged();
}

void ScopeController::setPreTriggerPercent(double v) {
    if (qFuzzyCompare(m_preTriggerPercent, v)) return;
    m_preTriggerPercent = v;
    if (m_testMode && m_signalGen) {
        m_signalGen->setPreTriggerPercent(v);
    }
    emit triggerChanged();
}

void ScopeController::forceTrigger() {
    if (m_testMode && m_signalGen) {
        m_signalGen->forceTrigger();
    } else if (m_engine) {
        m_engine->forceTrigger();
    }
}

void ScopeController::setTimebaseIndex(int v) {
    if (v < 0 || v >= s_timebaseSteps.size()) return;
    if (m_timebaseIndex == v) return;
    m_timebaseIndex = v;

    double secs = s_timebaseSteps[v];

    if (m_testMode && m_signalGen) {
        m_signalGen->setTimebaseSeconds(secs);
    } else if (m_engine) {
        uint32_t nsPerDiv = static_cast<uint32_t>(secs * 1e9);
        m_engine->setTimebase(nsPerDiv);
    }
    emit timebaseChanged();
}

void ScopeController::setHorizontalPosition(double v) {
    if (qFuzzyCompare(m_horizontalPosition, v)) return;
    m_horizontalPosition = v;
    if (m_testMode && m_signalGen) {
        m_signalGen->setHorizontalPosition(v);
    }
    emit horizontalChanged();
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
    if (m_testMode && m_signalGen) {
        m_signalGen->setCh1Offset(v);
    } else if (m_engine) {
        m_engine->setChannelOffset(0, static_cast<int16_t>(v * 1000));
    }
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
    if (m_testMode && m_signalGen) {
        m_signalGen->setCh2Offset(v);
    } else if (m_engine) {
        m_engine->setChannelOffset(1, static_cast<int16_t>(v * 1000));
    }
    emit channelChanged();
}

void ScopeController::setAcquisitionMode(int v) {
    if (m_acquisitionMode == v) return;
    m_acquisitionMode = v;
    if (m_testMode && m_signalGen) {
        m_signalGen->setAcquisitionMode(v);
    }
    emit acquisitionModeChanged();
}

// --- Simulation config setters (CH1) ---

void ScopeController::setCh1Waveform(int v) {
    if (static_cast<int>(m_ch1SimConfig.waveform) == v) return;
    m_ch1SimConfig.waveform = static_cast<WaveformType>(v);
    if (m_testMode && m_signalGen) m_signalGen->setCh1Waveform(v);
    emit simulationChanged();
}

void ScopeController::setCh1Amplitude(double v) {
    if (qFuzzyCompare(m_ch1SimConfig.amplitude, v)) return;
    m_ch1SimConfig.amplitude = v;
    if (m_testMode && m_signalGen) m_signalGen->setCh1Amplitude(v);
    emit simulationChanged();
}

void ScopeController::setCh1Frequency(double v) {
    if (qFuzzyCompare(m_ch1SimConfig.frequency, v)) return;
    m_ch1SimConfig.frequency = v;
    if (m_testMode && m_signalGen) m_signalGen->setCh1Frequency(v);
    emit simulationChanged();
}

void ScopeController::setCh1Phase(double v) {
    if (qFuzzyCompare(m_ch1SimConfig.phase, v)) return;
    m_ch1SimConfig.phase = v;
    if (m_testMode && m_signalGen) m_signalGen->setCh1Phase(v);
    emit simulationChanged();
}

void ScopeController::setCh1DcOffset(double v) {
    if (qFuzzyCompare(m_ch1SimConfig.dcOffset, v)) return;
    m_ch1SimConfig.dcOffset = v;
    if (m_testMode && m_signalGen) m_signalGen->setCh1DcOffset(v);
    emit simulationChanged();
}

void ScopeController::setCh1DutyCycle(double v) {
    if (qFuzzyCompare(m_ch1SimConfig.dutyCycle, v)) return;
    m_ch1SimConfig.dutyCycle = v;
    if (m_testMode && m_signalGen) m_signalGen->setCh1DutyCycle(v);
    emit simulationChanged();
}

void ScopeController::setCh1Noise(double v) {
    if (qFuzzyCompare(m_ch1SimConfig.noise, v)) return;
    m_ch1SimConfig.noise = v;
    if (m_testMode && m_signalGen) m_signalGen->setCh1Noise(v);
    emit simulationChanged();
}

// --- Simulation config setters (CH2) ---

void ScopeController::setCh2Waveform(int v) {
    if (static_cast<int>(m_ch2SimConfig.waveform) == v) return;
    m_ch2SimConfig.waveform = static_cast<WaveformType>(v);
    if (m_testMode && m_signalGen) m_signalGen->setCh2Waveform(v);
    emit simulationChanged();
}

void ScopeController::setCh2Amplitude(double v) {
    if (qFuzzyCompare(m_ch2SimConfig.amplitude, v)) return;
    m_ch2SimConfig.amplitude = v;
    if (m_testMode && m_signalGen) m_signalGen->setCh2Amplitude(v);
    emit simulationChanged();
}

void ScopeController::setCh2Frequency(double v) {
    if (qFuzzyCompare(m_ch2SimConfig.frequency, v)) return;
    m_ch2SimConfig.frequency = v;
    if (m_testMode && m_signalGen) m_signalGen->setCh2Frequency(v);
    emit simulationChanged();
}

void ScopeController::setCh2Phase(double v) {
    if (qFuzzyCompare(m_ch2SimConfig.phase, v)) return;
    m_ch2SimConfig.phase = v;
    if (m_testMode && m_signalGen) m_signalGen->setCh2Phase(v);
    emit simulationChanged();
}

void ScopeController::setCh2DcOffset(double v) {
    if (qFuzzyCompare(m_ch2SimConfig.dcOffset, v)) return;
    m_ch2SimConfig.dcOffset = v;
    if (m_testMode && m_signalGen) m_signalGen->setCh2DcOffset(v);
    emit simulationChanged();
}

void ScopeController::setCh2DutyCycle(double v) {
    if (qFuzzyCompare(m_ch2SimConfig.dutyCycle, v)) return;
    m_ch2SimConfig.dutyCycle = v;
    if (m_testMode && m_signalGen) m_signalGen->setCh2DutyCycle(v);
    emit simulationChanged();
}

void ScopeController::setCh2Noise(double v) {
    if (qFuzzyCompare(m_ch2SimConfig.noise, v)) return;
    m_ch2SimConfig.noise = v;
    if (m_testMode && m_signalGen) m_signalGen->setCh2Noise(v);
    emit simulationChanged();
}

// --- Direct series update from waveform buffer ---

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

    // Wire trigger status from generator to controller
    connect(m_signalGen.get(), &SignalGenerator::triggerStatusChanged,
            this, [this](const QString& status) {
        if (m_triggerStatus != status) {
            m_triggerStatus = status;
            emit triggerStatusChanged();
        }
    }, Qt::QueuedConnection);

    // Apply current state
    m_signalGen->setRunning(true);
    m_signalGen->setTriggerLevel(m_triggerLevel);
    m_signalGen->setTriggerSource(m_triggerSource);
    m_signalGen->setTriggerEdge(m_triggerEdge);
    m_signalGen->setTriggerHoldoff(m_triggerHoldoff);
    m_signalGen->setPreTriggerPercent(m_preTriggerPercent);
    m_signalGen->setAcquisitionMode(m_acquisitionMode);

    // Timebase
    m_signalGen->setTimebaseSeconds(s_timebaseSteps[m_timebaseIndex]);
    m_signalGen->setHorizontalPosition(m_horizontalPosition);

    // Channel offsets
    m_signalGen->setCh1Offset(m_ch1Offset);
    m_signalGen->setCh2Offset(m_ch2Offset);

    // Simulation config CH1
    m_signalGen->setCh1Waveform(static_cast<int>(m_ch1SimConfig.waveform));
    m_signalGen->setCh1Amplitude(m_ch1SimConfig.amplitude);
    m_signalGen->setCh1Frequency(m_ch1SimConfig.frequency);
    m_signalGen->setCh1Phase(m_ch1SimConfig.phase);
    m_signalGen->setCh1DcOffset(m_ch1SimConfig.dcOffset);
    m_signalGen->setCh1DutyCycle(m_ch1SimConfig.dutyCycle);
    m_signalGen->setCh1Noise(m_ch1SimConfig.noise);

    // Simulation config CH2
    m_signalGen->setCh2Waveform(static_cast<int>(m_ch2SimConfig.waveform));
    m_signalGen->setCh2Amplitude(m_ch2SimConfig.amplitude);
    m_signalGen->setCh2Frequency(m_ch2SimConfig.frequency);
    m_signalGen->setCh2Phase(m_ch2SimConfig.phase);
    m_signalGen->setCh2DcOffset(m_ch2SimConfig.dcOffset);
    m_signalGen->setCh2DutyCycle(m_ch2SimConfig.dutyCycle);
    m_signalGen->setCh2Noise(m_ch2SimConfig.noise);

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
