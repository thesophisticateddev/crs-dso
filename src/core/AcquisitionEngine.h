#pragma once
#include <QThread>
#include <QMutex>
#include <atomic>
#include <memory>
#include "transport/ITransport.h"
#include "protocol/ProtocolCodec.h"
#include "core/DeviceSettings.h"
#include "core/WaveformBuffer.h"

enum class EngineState {
    Disconnected, Idle, Armed, Acquiring, Stopped, Error
};

enum class AcquisitionMode {
    Auto, Normal, Single
};

class AcquisitionEngine : public QThread {
    Q_OBJECT

public:
    explicit AcquisitionEngine(std::unique_ptr<ITransport> transport,
                               QObject* parent = nullptr);
    ~AcquisitionEngine() override;

    // Thread-safe state queries
    EngineState currentState() const;

signals:
    void stateChanged(EngineState newState);
    void waveformReady(std::shared_ptr<WaveformBuffer> buffer);
    void deviceIdentified(const QString& model, const QString& firmware);
    void errorOccurred(const QString& message);
    void connectionChanged(bool connected);

public slots:
    // All slots are thread-safe — they queue commands to the engine thread
    void connectDevice(const DeviceInfo& info);
    void disconnectDevice();

    void setSampleRate(uint32_t samplesPerSec);
    void setVoltageRange(uint8_t channel, uint8_t rangeIndex);
    void setTrigger(uint8_t source, uint8_t edge, int16_t levelMv);
    void setTimebase(uint32_t nsPerDiv);
    void setCoupling(uint8_t channel, uint8_t mode);
    void setChannelEnabled(uint8_t channel, bool enabled);
    void setChannelOffset(uint8_t channel, int16_t offsetMv);

    void startAcquisition(AcquisitionMode mode);
    void stopAcquisition();
    void forceTrigger();

    void requestStop();

protected:
    void run() override;  // Main acquisition loop

private:
    std::unique_ptr<ITransport> m_transport;
    ProtocolCodec m_codec;
    std::atomic<EngineState> m_state{EngineState::Disconnected};
    std::atomic<bool> m_stopRequested{false};

    // Command queue (UI thread -> engine thread)
    struct Command { uint8_t id; std::vector<uint8_t> payload; };
    QMutex m_cmdMutex;
    std::vector<Command> m_pendingCommands;

    void processIncoming();
    void processPendingCommands();
    void handleFrame(const ProtocolFrame& frame);
    void sendCommand(uint8_t cmdId,
                     const std::vector<uint8_t>& payload = {});
    void setState(EngineState s);
    void queueCommand(uint8_t cmdId, const std::vector<uint8_t>& payload = {});
};
