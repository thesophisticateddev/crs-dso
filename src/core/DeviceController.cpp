#include "DeviceController.h"
#include "transport/MockTransport.h"
#ifdef HAS_LIBUSB
#include "transport/UsbTransport.h"
#endif
#include <QCoreApplication>
#include <QPointF>

DeviceController::DeviceController(QObject* parent)
    : QObject(parent)
{
}

DeviceController::~DeviceController() {
    if (m_engine) {
        m_engine->requestStop();
        m_engine->wait(3000);
    }
}

void DeviceController::createEngine(bool useMock) {
    std::unique_ptr<ITransport> transport;
    if (useMock) {
        transport = std::make_unique<MockTransport>();
    } else {
#ifdef HAS_LIBUSB
        transport = std::make_unique<UsbTransport>();
#else
        // No libusb available, fall back to mock
        transport = std::make_unique<MockTransport>();
#endif
    }

    m_engine = std::make_unique<AcquisitionEngine>(std::move(transport));

    connect(m_engine.get(), &AcquisitionEngine::waveformReady,
            this, &DeviceController::onWaveformReady, Qt::QueuedConnection);
    connect(m_engine.get(), &AcquisitionEngine::stateChanged,
            this, &DeviceController::onEngineStateChanged, Qt::QueuedConnection);
    connect(m_engine.get(), &AcquisitionEngine::errorOccurred,
            this, &DeviceController::onEngineError, Qt::QueuedConnection);
    connect(m_engine.get(), &AcquisitionEngine::deviceIdentified,
            this, &DeviceController::onDeviceIdentified, Qt::QueuedConnection);
    connect(m_engine.get(), &AcquisitionEngine::connectionChanged,
            this, [this](bool conn) {
                m_connected = conn;
                emit connectionChanged();
            }, Qt::QueuedConnection);
}

// --- QML-callable methods ---

void DeviceController::scanDevices() {
    // Use a temporary transport to enumerate
    bool useMock = QCoreApplication::arguments().contains("--demoMode");
    if (useMock) {
        MockTransport mock;
        m_discoveredDevices = mock.enumerate();
    } else {
#ifdef HAS_LIBUSB
        UsbTransport usb;
        m_discoveredDevices = usb.enumerate();
#else
        MockTransport mock;
        m_discoveredDevices = mock.enumerate();
#endif
    }

    QVariantList deviceList;
    for (const auto& dev : m_discoveredDevices) {
        QVariantMap map;
        map["vendorId"] = dev.vendorId;
        map["productId"] = dev.productId;
        map["serial"] = QString::fromStdString(dev.serial);
        map["manufacturer"] = QString::fromStdString(dev.manufacturer);
        map["product"] = QString::fromStdString(dev.product);
        deviceList.append(map);
    }

    emit devicesScanned(deviceList);
}

QVariantList DeviceController::availableDevices() const {
    QVariantList deviceList;
    for (const auto& dev : m_discoveredDevices) {
        QVariantMap map;
        map["vendorId"] = dev.vendorId;
        map["productId"] = dev.productId;
        map["serial"] = QString::fromStdString(dev.serial);
        map["manufacturer"] = QString::fromStdString(dev.manufacturer);
        map["product"] = QString::fromStdString(dev.product);
        deviceList.append(map);
    }
    return deviceList;
}

void DeviceController::connectToDevice(int index) {
    if (index < 0 || index >= static_cast<int>(m_discoveredDevices.size())) {
        emit errorMessage("Invalid device index");
        return;
    }

    bool useMock = QCoreApplication::arguments().contains("--demoMode");
    createEngine(useMock);
    m_engine->connectDevice(m_discoveredDevices[index]);
}

void DeviceController::disconnect() {
    if (m_engine) {
        m_engine->disconnectDevice();
        m_engine.reset();
    }
    m_connected = false;
    m_engineState = EngineState::Disconnected;
    emit connectionChanged();
    emit stateChanged();
}

void DeviceController::startAcquisition(int mode) {
    if (m_engine) {
        m_engine->startAcquisition(static_cast<AcquisitionMode>(mode));
    }
}

void DeviceController::stopAcquisition() {
    if (m_engine) {
        m_engine->stopAcquisition();
    }
}

void DeviceController::forceTrigger() {
    if (m_engine) {
        m_engine->forceTrigger();
    }
}

void DeviceController::autoSet() {
    // Auto-configure based on detected signal
    // For now, set reasonable defaults
    setSampleRate(1000000);
    setTimebaseNsPerDiv(1000);
    setCh1Enabled(true);
    emit settingsChanged();
}

// --- Property getters ---

bool DeviceController::connected() const { return m_connected; }

QString DeviceController::deviceName() const { return m_deviceName; }

QString DeviceController::firmwareVersion() const { return m_firmwareVersion; }

QString DeviceController::stateString() const {
    switch (m_engineState) {
    case EngineState::Disconnected: return QStringLiteral("Disconnected");
    case EngineState::Idle:         return QStringLiteral("Idle");
    case EngineState::Armed:        return QStringLiteral("Armed");
    case EngineState::Acquiring:    return QStringLiteral("Acquiring");
    case EngineState::Stopped:      return QStringLiteral("Stopped");
    case EngineState::Error:        return QStringLiteral("Error");
    }
    return QStringLiteral("Unknown");
}

bool DeviceController::isRunning() const {
    return m_engineState == EngineState::Armed ||
           m_engineState == EngineState::Acquiring;
}

// --- Channel 1 getters/setters ---
bool DeviceController::ch1Enabled() const { return m_settings.ch1Enabled; }
void DeviceController::setCh1Enabled(bool v) {
    if (m_settings.ch1Enabled != v) {
        m_settings.ch1Enabled = v;
        if (m_engine) m_engine->setChannelEnabled(0, v);
        emit settingsChanged();
    }
}

int DeviceController::ch1VoltageRange() const { return m_settings.ch1VoltageRange; }
void DeviceController::setCh1VoltageRange(int v) {
    if (m_settings.ch1VoltageRange != v) {
        m_settings.ch1VoltageRange = static_cast<uint8_t>(v);
        if (m_engine) m_engine->setVoltageRange(0, static_cast<uint8_t>(v));
        emit settingsChanged();
    }
}

int DeviceController::ch1Coupling() const { return m_settings.ch1Coupling; }
void DeviceController::setCh1Coupling(int v) {
    if (m_settings.ch1Coupling != v) {
        m_settings.ch1Coupling = static_cast<uint8_t>(v);
        if (m_engine) m_engine->setCoupling(0, static_cast<uint8_t>(v));
        emit settingsChanged();
    }
}

int DeviceController::ch1Offset() const { return m_settings.ch1Offset; }
void DeviceController::setCh1Offset(int v) {
    if (m_settings.ch1Offset != v) {
        m_settings.ch1Offset = static_cast<int16_t>(v);
        if (m_engine) m_engine->setChannelOffset(0, static_cast<int16_t>(v));
        emit settingsChanged();
    }
}

// --- Channel 2 getters/setters ---
bool DeviceController::ch2Enabled() const { return m_settings.ch2Enabled; }
void DeviceController::setCh2Enabled(bool v) {
    if (m_settings.ch2Enabled != v) {
        m_settings.ch2Enabled = v;
        if (m_engine) m_engine->setChannelEnabled(1, v);
        emit settingsChanged();
    }
}

int DeviceController::ch2VoltageRange() const { return m_settings.ch2VoltageRange; }
void DeviceController::setCh2VoltageRange(int v) {
    if (m_settings.ch2VoltageRange != v) {
        m_settings.ch2VoltageRange = static_cast<uint8_t>(v);
        if (m_engine) m_engine->setVoltageRange(1, static_cast<uint8_t>(v));
        emit settingsChanged();
    }
}

int DeviceController::ch2Coupling() const { return m_settings.ch2Coupling; }
void DeviceController::setCh2Coupling(int v) {
    if (m_settings.ch2Coupling != v) {
        m_settings.ch2Coupling = static_cast<uint8_t>(v);
        if (m_engine) m_engine->setCoupling(1, static_cast<uint8_t>(v));
        emit settingsChanged();
    }
}

int DeviceController::ch2Offset() const { return m_settings.ch2Offset; }
void DeviceController::setCh2Offset(int v) {
    if (m_settings.ch2Offset != v) {
        m_settings.ch2Offset = static_cast<int16_t>(v);
        if (m_engine) m_engine->setChannelOffset(1, static_cast<int16_t>(v));
        emit settingsChanged();
    }
}

// --- Timebase / trigger getters/setters ---
int DeviceController::sampleRate() const { return static_cast<int>(m_settings.sampleRate); }
void DeviceController::setSampleRate(int v) {
    if (m_settings.sampleRate != static_cast<uint32_t>(v)) {
        m_settings.sampleRate = static_cast<uint32_t>(v);
        if (m_engine) m_engine->setSampleRate(static_cast<uint32_t>(v));
        emit settingsChanged();
    }
}

int DeviceController::timebaseNsPerDiv() const { return static_cast<int>(m_settings.timebaseNsPerDiv); }
void DeviceController::setTimebaseNsPerDiv(int v) {
    if (m_settings.timebaseNsPerDiv != static_cast<uint32_t>(v)) {
        m_settings.timebaseNsPerDiv = static_cast<uint32_t>(v);
        if (m_engine) m_engine->setTimebase(static_cast<uint32_t>(v));
        emit settingsChanged();
    }
}

int DeviceController::triggerSource() const { return m_settings.triggerSource; }
void DeviceController::setTriggerSource(int v) {
    if (m_settings.triggerSource != v) {
        m_settings.triggerSource = static_cast<uint8_t>(v);
        if (m_engine) m_engine->setTrigger(static_cast<uint8_t>(v),
                                            m_settings.triggerEdge,
                                            m_settings.triggerLevel);
        emit settingsChanged();
    }
}

int DeviceController::triggerEdge() const { return m_settings.triggerEdge; }
void DeviceController::setTriggerEdge(int v) {
    if (m_settings.triggerEdge != v) {
        m_settings.triggerEdge = static_cast<uint8_t>(v);
        if (m_engine) m_engine->setTrigger(m_settings.triggerSource,
                                            static_cast<uint8_t>(v),
                                            m_settings.triggerLevel);
        emit settingsChanged();
    }
}

int DeviceController::triggerLevel() const { return m_settings.triggerLevel; }
void DeviceController::setTriggerLevel(int v) {
    if (m_settings.triggerLevel != v) {
        m_settings.triggerLevel = static_cast<int16_t>(v);
        if (m_engine) m_engine->setTrigger(m_settings.triggerSource,
                                            m_settings.triggerEdge,
                                            static_cast<int16_t>(v));
        emit settingsChanged();
    }
}

// --- Waveform data ---

QVariantList DeviceController::ch1Data() const {
    QVariantList points;
    if (!m_currentWaveform || m_currentWaveform->channels.empty())
        return points;

    auto voltages = m_currentWaveform->voltageData(0);
    points.reserve(static_cast<int>(voltages.size()));
    for (size_t i = 0; i < voltages.size(); i++) {
        points.append(QVariant::fromValue(QPointF(static_cast<qreal>(i), voltages[i])));
    }
    return points;
}

QVariantList DeviceController::ch2Data() const {
    QVariantList points;
    if (!m_currentWaveform || m_currentWaveform->channels.size() < 2)
        return points;

    auto voltages = m_currentWaveform->voltageData(1);
    points.reserve(static_cast<int>(voltages.size()));
    for (size_t i = 0; i < voltages.size(); i++) {
        points.append(QVariant::fromValue(QPointF(static_cast<qreal>(i), voltages[i])));
    }
    return points;
}

// --- Private slots ---

void DeviceController::onWaveformReady(std::shared_ptr<WaveformBuffer> buffer) {
    m_currentWaveform = buffer;
    emit waveformUpdated();
}

void DeviceController::onEngineStateChanged(EngineState state) {
    m_engineState = state;
    emit stateChanged();
}

void DeviceController::onEngineError(const QString& msg) {
    emit errorMessage(msg);
}

void DeviceController::onDeviceIdentified(const QString& model, const QString& firmware) {
    m_deviceName = model;
    m_firmwareVersion = firmware;
    emit deviceInfoChanged();
}
