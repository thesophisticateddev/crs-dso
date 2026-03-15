#include "AcquisitionEngine.h"
#include "protocol/ProtocolDefs.h"
#include <QThread>
#include <cstring>

AcquisitionEngine::AcquisitionEngine(std::unique_ptr<ITransport> transport,
                                     QObject* parent)
    : QThread(parent)
    , m_transport(std::move(transport))
{
}

AcquisitionEngine::~AcquisitionEngine() {
    requestStop();
    if (isRunning()) {
        quit();
        wait(5000);
    }
    if (m_transport && m_transport->state() == TransportState::Connected) {
        m_transport->close();
    }
}

EngineState AcquisitionEngine::currentState() const {
    return m_state.load();
}

void AcquisitionEngine::connectDevice(const DeviceInfo& info) {
    if (m_transport->open(info)) {
        setState(EngineState::Idle);
        emit connectionChanged(true);

        // Send IDENTIFY command
        queueCommand(Cmd::IDENTIFY);

        // Start the acquisition thread if not running
        if (!isRunning()) {
            m_stopRequested = false;
            start();
        }
    } else {
        setState(EngineState::Error);
        emit errorOccurred(QString::fromStdString(m_transport->lastError()));
    }
}

void AcquisitionEngine::disconnectDevice() {
    // Queue stop then close
    if (m_state == EngineState::Armed || m_state == EngineState::Acquiring) {
        queueCommand(Cmd::STOP_ACQUISITION);
    }
    requestStop();
    if (isRunning()) {
        wait(3000);
    }
    if (m_transport) {
        m_transport->close();
    }
    setState(EngineState::Disconnected);
    emit connectionChanged(false);
}

void AcquisitionEngine::setSampleRate(uint32_t samplesPerSec) {
    std::vector<uint8_t> payload(4);
    std::memcpy(payload.data(), &samplesPerSec, 4);
    queueCommand(Cmd::SET_SAMPLE_RATE, payload);
}

void AcquisitionEngine::setVoltageRange(uint8_t channel, uint8_t rangeIndex) {
    queueCommand(Cmd::SET_VOLTAGE_RANGE, {channel, rangeIndex});
}

void AcquisitionEngine::setTrigger(uint8_t source, uint8_t edge, int16_t levelMv) {
    std::vector<uint8_t> payload = {source, edge};
    payload.push_back(static_cast<uint8_t>(levelMv & 0xFF));
    payload.push_back(static_cast<uint8_t>((levelMv >> 8) & 0xFF));
    queueCommand(Cmd::SET_TRIGGER, payload);
}

void AcquisitionEngine::setTimebase(uint32_t nsPerDiv) {
    std::vector<uint8_t> payload(4);
    std::memcpy(payload.data(), &nsPerDiv, 4);
    queueCommand(Cmd::SET_TIMEBASE, payload);
}

void AcquisitionEngine::setCoupling(uint8_t channel, uint8_t mode) {
    queueCommand(Cmd::SET_COUPLING, {channel, mode});
}

void AcquisitionEngine::setChannelEnabled(uint8_t channel, bool enabled) {
    queueCommand(Cmd::SET_CHANNEL_ENABLE, {channel, static_cast<uint8_t>(enabled ? 1 : 0)});
}

void AcquisitionEngine::setChannelOffset(uint8_t channel, int16_t offsetMv) {
    std::vector<uint8_t> payload = {channel};
    payload.push_back(static_cast<uint8_t>(offsetMv & 0xFF));
    payload.push_back(static_cast<uint8_t>((offsetMv >> 8) & 0xFF));
    queueCommand(Cmd::SET_CHANNEL_OFFSET, payload);
}

void AcquisitionEngine::startAcquisition(AcquisitionMode mode) {
    queueCommand(Cmd::START_ACQUISITION, {static_cast<uint8_t>(mode)});
    setState(EngineState::Armed);
}

void AcquisitionEngine::stopAcquisition() {
    queueCommand(Cmd::STOP_ACQUISITION);
    setState(EngineState::Stopped);
}

void AcquisitionEngine::forceTrigger() {
    queueCommand(Cmd::FORCE_TRIGGER);
}

void AcquisitionEngine::requestStop() {
    m_stopRequested = true;
}

void AcquisitionEngine::run() {
    // Main acquisition loop
    while (!m_stopRequested) {
        if (m_transport->state() != TransportState::Connected) {
            QThread::msleep(100);
            continue;
        }

        // Send pending commands
        processPendingCommands();

        // Read incoming data
        processIncoming();

        // Small sleep to prevent busy-waiting
        QThread::msleep(1);
    }
}

void AcquisitionEngine::processIncoming() {
    uint8_t buffer[4096];
    int bytesRead = m_transport->read(buffer, sizeof(buffer), 10);

    if (bytesRead > 0) {
        m_codec.feedBytes(buffer, static_cast<size_t>(bytesRead));

        while (auto frame = m_codec.nextFrame()) {
            handleFrame(*frame);
        }
    }
}

void AcquisitionEngine::processPendingCommands() {
    std::vector<Command> commands;
    {
        QMutexLocker locker(&m_cmdMutex);
        commands.swap(m_pendingCommands);
    }

    for (const auto& cmd : commands) {
        sendCommand(cmd.id, cmd.payload);
    }
}

void AcquisitionEngine::handleFrame(const ProtocolFrame& frame) {
    switch (frame.commandId) {
    case Cmd::IDENTIFY_RESP: {
        // Parse model + firmware string
        std::string info(frame.payload.begin(), frame.payload.end());
        // Split at first null or use the whole string
        auto sep = info.find('\0');
        QString model = QString::fromStdString(sep != std::string::npos ? info.substr(0, sep) : info);
        QString firmware = QString::fromStdString(sep != std::string::npos ? info.substr(sep + 1) : "");
        emit deviceIdentified(model, firmware);
        break;
    }

    case Cmd::ACK:
        // Command acknowledged — could track per-command status
        break;

    case Cmd::NACK:
        if (frame.payload.size() >= 2) {
            emit errorOccurred(QString("Command 0x%1 rejected with error code %2")
                .arg(frame.payload[0], 2, 16, QChar('0'))
                .arg(frame.payload[1]));
        }
        break;

    case Cmd::WAVEFORM_DATA: {
        if (frame.payload.size() < sizeof(WaveformHeader))
            break;

        WaveformHeader header;
        std::memcpy(&header, frame.payload.data(), sizeof(WaveformHeader));

        auto waveform = std::make_shared<WaveformBuffer>();
        waveform->channelMask = header.channelMask;
        waveform->sampleRate = header.sampleRateHz;
        waveform->triggerIndex = header.triggerIndex;
        waveform->voltageScale = header.voltageScale;
        waveform->bitsPerSample = header.bitsPerSample;

        // Count active channels
        int numChannels = 0;
        for (int bit = 0; bit < 8; bit++) {
            if (header.channelMask & (1 << bit))
                numChannels++;
        }

        if (numChannels > 0 && header.numSamples > 0) {
            const int16_t* sampleData = reinterpret_cast<const int16_t*>(
                frame.payload.data() + sizeof(WaveformHeader));
            size_t totalSamples = (frame.payload.size() - sizeof(WaveformHeader)) / sizeof(int16_t);

            waveform->channels.resize(numChannels);
            size_t samplesPerChannel = std::min(
                static_cast<size_t>(header.numSamples),
                totalSamples / numChannels);

            for (int ch = 0; ch < numChannels; ch++) {
                waveform->channels[ch].assign(
                    sampleData + ch * samplesPerChannel,
                    sampleData + (ch + 1) * samplesPerChannel);
            }
        }

        setState(EngineState::Acquiring);
        emit waveformReady(waveform);
        break;
    }

    case Cmd::TRIGGER_EVENT:
        setState(EngineState::Acquiring);
        break;

    case Cmd::DEVICE_STATUS_RESP:
        // Parse device status — application-specific
        break;

    case Cmd::ERROR_REPORT: {
        if (!frame.payload.empty()) {
            uint8_t errorType = frame.payload[0];
            std::string msg(frame.payload.begin() + 1, frame.payload.end());
            emit errorOccurred(QString("Device error (type %1): %2")
                .arg(errorType).arg(QString::fromStdString(msg)));
        }
        break;
    }

    default:
        break;
    }
}

void AcquisitionEngine::sendCommand(uint8_t cmdId, const std::vector<uint8_t>& payload) {
    ProtocolFrame frame;
    frame.commandId = cmdId;
    frame.payload = payload;

    auto encoded = ProtocolCodec::encode(frame);
    m_transport->write(encoded.data(), encoded.size());
}

void AcquisitionEngine::setState(EngineState s) {
    EngineState old = m_state.exchange(s);
    if (old != s) {
        emit stateChanged(s);
    }
}

void AcquisitionEngine::queueCommand(uint8_t cmdId, const std::vector<uint8_t>& payload) {
    QMutexLocker locker(&m_cmdMutex);
    m_pendingCommands.push_back({cmdId, payload});
}
