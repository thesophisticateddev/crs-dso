#pragma once
#include <QObject>
#include <QVariantList>
#include <QPointF>
#include <memory>
#include "AcquisitionEngine.h"

class DeviceController : public QObject {
    Q_OBJECT

    // Connection state
    Q_PROPERTY(bool connected READ connected NOTIFY connectionChanged)
    Q_PROPERTY(QString deviceName READ deviceName NOTIFY deviceInfoChanged)
    Q_PROPERTY(QString firmwareVersion READ firmwareVersion
               NOTIFY deviceInfoChanged)

    // Acquisition state
    Q_PROPERTY(QString state READ stateString NOTIFY stateChanged)
    Q_PROPERTY(bool running READ isRunning NOTIFY stateChanged)

    // Channel 1 settings
    Q_PROPERTY(bool ch1Enabled READ ch1Enabled WRITE setCh1Enabled
               NOTIFY settingsChanged)
    Q_PROPERTY(int ch1VoltageRange READ ch1VoltageRange
               WRITE setCh1VoltageRange NOTIFY settingsChanged)
    Q_PROPERTY(int ch1Coupling READ ch1Coupling WRITE setCh1Coupling
               NOTIFY settingsChanged)
    Q_PROPERTY(int ch1Offset READ ch1Offset WRITE setCh1Offset
               NOTIFY settingsChanged)

    // Channel 2 settings
    Q_PROPERTY(bool ch2Enabled READ ch2Enabled WRITE setCh2Enabled
               NOTIFY settingsChanged)
    Q_PROPERTY(int ch2VoltageRange READ ch2VoltageRange
               WRITE setCh2VoltageRange NOTIFY settingsChanged)
    Q_PROPERTY(int ch2Coupling READ ch2Coupling WRITE setCh2Coupling
               NOTIFY settingsChanged)
    Q_PROPERTY(int ch2Offset READ ch2Offset WRITE setCh2Offset
               NOTIFY settingsChanged)

    // Timebase and trigger
    Q_PROPERTY(int sampleRate READ sampleRate WRITE setSampleRate
               NOTIFY settingsChanged)
    Q_PROPERTY(int timebaseNsPerDiv READ timebaseNsPerDiv
               WRITE setTimebaseNsPerDiv NOTIFY settingsChanged)
    Q_PROPERTY(int triggerSource READ triggerSource WRITE setTriggerSource
               NOTIFY settingsChanged)
    Q_PROPERTY(int triggerEdge READ triggerEdge WRITE setTriggerEdge
               NOTIFY settingsChanged)
    Q_PROPERTY(int triggerLevel READ triggerLevel WRITE setTriggerLevel
               NOTIFY settingsChanged)

    // Waveform data for QML Charts
    Q_PROPERTY(QVariantList ch1Data READ ch1Data NOTIFY waveformUpdated)
    Q_PROPERTY(QVariantList ch2Data READ ch2Data NOTIFY waveformUpdated)

public:
    explicit DeviceController(QObject* parent = nullptr);
    ~DeviceController() override;

    // QML-callable methods
    Q_INVOKABLE void scanDevices();
    Q_INVOKABLE QVariantList availableDevices() const;
    Q_INVOKABLE void connectToDevice(int index);
    Q_INVOKABLE void disconnect();

    Q_INVOKABLE void startAcquisition(int mode);  // 0=Auto, 1=Normal, 2=Single
    Q_INVOKABLE void stopAcquisition();
    Q_INVOKABLE void forceTrigger();
    Q_INVOKABLE void autoSet();

    // Property getters
    bool connected() const;
    QString deviceName() const;
    QString firmwareVersion() const;
    QString stateString() const;
    bool isRunning() const;

    // Channel 1
    bool ch1Enabled() const;
    void setCh1Enabled(bool v);
    int ch1VoltageRange() const;
    void setCh1VoltageRange(int v);
    int ch1Coupling() const;
    void setCh1Coupling(int v);
    int ch1Offset() const;
    void setCh1Offset(int v);

    // Channel 2
    bool ch2Enabled() const;
    void setCh2Enabled(bool v);
    int ch2VoltageRange() const;
    void setCh2VoltageRange(int v);
    int ch2Coupling() const;
    void setCh2Coupling(int v);
    int ch2Offset() const;
    void setCh2Offset(int v);

    // Timebase / trigger
    int sampleRate() const;
    void setSampleRate(int v);
    int timebaseNsPerDiv() const;
    void setTimebaseNsPerDiv(int v);
    int triggerSource() const;
    void setTriggerSource(int v);
    int triggerEdge() const;
    void setTriggerEdge(int v);
    int triggerLevel() const;
    void setTriggerLevel(int v);

    // Waveform data
    QVariantList ch1Data() const;
    QVariantList ch2Data() const;

signals:
    void connectionChanged();
    void deviceInfoChanged();
    void stateChanged();
    void settingsChanged();
    void waveformUpdated();
    void errorMessage(const QString& msg);
    void devicesScanned(const QVariantList& devices);

private slots:
    void onWaveformReady(std::shared_ptr<WaveformBuffer> buffer);
    void onEngineStateChanged(EngineState state);
    void onEngineError(const QString& msg);
    void onDeviceIdentified(const QString& model, const QString& firmware);

private:
    void createEngine(bool useMock);

    std::unique_ptr<AcquisitionEngine> m_engine;
    std::shared_ptr<WaveformBuffer> m_currentWaveform;
    std::vector<DeviceInfo> m_discoveredDevices;

    // Cached settings
    DeviceSettings m_settings;

    // Device info
    bool m_connected = false;
    QString m_deviceName;
    QString m_firmwareVersion;
    EngineState m_engineState = EngineState::Disconnected;
};
