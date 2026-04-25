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
    // Queue a clean disconnect if still connected, then stop the thread
    if (m_transport && m_transport->state() == TransportState::Connected) {
        queueCommand(InternalCmd::DISCONNECT);
    }
    requestStop();
    if (isRunning()) {
        quit();
        wait(5000);
    }
}

EngineState AcquisitionEngine::currentState() const {
    return m_state.load();
}

void AcquisitionEngine::connectDevice(const DeviceInfo& info) {
    // Queue the connect to run on the engine thread
    {
        QMutexLocker locker(&m_cmdMutex);
        m_pendingDeviceInfo = info;
    }

    // Start the acquisition thread if not running
    if (!isRunning()) {
        m_stopRequested = false;
        start();
    }

    queueCommand(InternalCmd::CONNECT);
}

void AcquisitionEngine::disconnectDevice() {
    // Queue stop + disconnect to run on the engine thread
    if (m_state == EngineState::Armed || m_state == EngineState::Acquiring) {
        queueCommand(Cmd::STOP_ACQUISITION);
    }
    queueCommand(InternalCmd::DISCONNECT);

    // Wait for thread to process the disconnect and stop
    requestStop();
    if (isRunning()) {
        wait(3000);
    }
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
        // Always process pending commands (including internal connect/disconnect)
        processPendingCommands();

        if (m_transport->state() != TransportState::Connected) {
            QThread::msleep(100);
            continue;
        }

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
        if (cmd.id == InternalCmd::CONNECT || cmd.id == InternalCmd::DISCONNECT) {
            handleInternalCommand(cmd);
        } else {
            sendCommand(cmd.id, cmd.payload);
        }
    }
}

void AcquisitionEngine::handleInternalCommand(const Command& cmd) {
    if (cmd.id == InternalCmd::CONNECT) {
        DeviceInfo info;
        {
            QMutexLocker locker(&m_cmdMutex);
            info = m_pendingDeviceInfo;
        }
        if (m_transport->open(info)) {
            setState(EngineState::Idle);
            emit connectionChanged(true);
            sendCommand(Cmd::IDENTIFY);
        } else {
            setState(EngineState::Error);
            emit errorOccurred(QString::fromStdString(m_transport->lastError()));
        }
    } else if (cmd.id == InternalCmd::DISCONNECT) {
        if (m_transport) {
            m_transport->close();
        }
        setState(EngineState::Disconnected);
        emit connectionChanged(false);
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

        // Validate bitsPerSample — only 8 and 16 are currently supported
        if (header.bitsPerSample != 8 && header.bitsPerSample != 16) {
            emit errorOccurred(QString("Unsupported sample width: %1 bits")
                .arg(header.bitsPerSample));
            break;
        }

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

        const uint8_t* rawData = frame.payload.data() + sizeof(WaveformHeader);
        size_t rawBytes = frame.payload.size() - sizeof(WaveformHeader);

        if (numChannels > 0 && header.numSamples > 0) {
            waveform->channels.resize(numChannels);

            if (header.bitsPerSample == 16) {
                size_t totalSamples = rawBytes / sizeof(int16_t);
                size_t samplesPerChannel = std::min(
                    static_cast<size_t>(header.numSamples),
                    totalSamples / numChannels);

                const int16_t* sampleData = reinterpret_cast<const int16_t*>(rawData);
                for (int ch = 0; ch < numChannels; ch++) {
                    waveform->channels[ch].assign(
                        sampleData + ch * samplesPerChannel,
                        sampleData + (ch + 1) * samplesPerChannel);
                }
            } else if (header.bitsPerSample == 8) {
                size_t totalSamples = rawBytes;
                size_t samplesPerChannel = std::min(
                    static_cast<size_t>(header.numSamples),
                    totalSamples / numChannels);

                for (int ch = 0; ch < numChannels; ch++) {
                    const uint8_t* src = rawData + ch * samplesPerChannel;
                    waveform->channels[ch].resize(samplesPerChannel);
                    for (size_t i = 0; i < samplesPerChannel; i++) {
                        // Sign-extend 8-bit sample to int16_t
                        waveform->channels[ch][i] = static_cast<int16_t>(
                            static_cast<int8_t>(src[i]));
                    }
                }
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
