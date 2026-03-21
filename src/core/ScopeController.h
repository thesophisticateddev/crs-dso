#pragma once
#include <QObject>
#include <QVariantList>
#include <QStringList>
#include <QtCharts/QXYSeries>
#include <memory>
#include "signalgenerator.h"

class AcquisitionEngine;
struct WaveformBuffer;
struct DeviceInfo;

class ScopeController : public QObject {
    Q_OBJECT

    // Mode
    Q_PROPERTY(bool testMode READ testMode WRITE setTestMode NOTIFY modeChanged)
    Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)

    // Trigger (unified — QML never touches signalGenerator directly)
    Q_PROPERTY(int triggerSource READ triggerSource WRITE setTriggerSource
               NOTIFY triggerChanged)
    Q_PROPERTY(int triggerEdge READ triggerEdge WRITE setTriggerEdge
               NOTIFY triggerChanged)
    Q_PROPERTY(double triggerLevel READ triggerLevel WRITE setTriggerLevel
               NOTIFY triggerChanged)
    Q_PROPERTY(double triggerHoldoff READ triggerHoldoff WRITE setTriggerHoldoff
               NOTIFY triggerChanged)
    Q_PROPERTY(double preTriggerPercent READ preTriggerPercent WRITE setPreTriggerPercent
               NOTIFY triggerChanged)
    Q_PROPERTY(QString triggerStatus READ triggerStatus NOTIFY triggerStatusChanged)

    // Timebase (using step index, not raw value)
    Q_PROPERTY(int timebaseIndex READ timebaseIndex WRITE setTimebaseIndex
               NOTIFY timebaseChanged)
    Q_PROPERTY(QString timebaseLabel READ timebaseLabel NOTIFY timebaseChanged)

    // Horizontal position
    Q_PROPERTY(double horizontalPosition READ horizontalPosition
               WRITE setHorizontalPosition NOTIFY horizontalChanged)

    // Channel 1
    Q_PROPERTY(bool ch1Enabled READ ch1Enabled WRITE setCh1Enabled
               NOTIFY channelChanged)
    Q_PROPERTY(int ch1VoltsIndex READ ch1VoltsIndex WRITE setCh1VoltsIndex
               NOTIFY channelChanged)
    Q_PROPERTY(int ch1Coupling READ ch1Coupling WRITE setCh1Coupling
               NOTIFY channelChanged)
    Q_PROPERTY(double ch1Offset READ ch1Offset WRITE setCh1Offset
               NOTIFY channelChanged)

    // Channel 2 (mirrors CH1 properties)
    Q_PROPERTY(bool ch2Enabled READ ch2Enabled WRITE setCh2Enabled
               NOTIFY channelChanged)
    Q_PROPERTY(int ch2VoltsIndex READ ch2VoltsIndex WRITE setCh2VoltsIndex
               NOTIFY channelChanged)
    Q_PROPERTY(int ch2Coupling READ ch2Coupling WRITE setCh2Coupling
               NOTIFY channelChanged)
    Q_PROPERTY(double ch2Offset READ ch2Offset WRITE setCh2Offset
               NOTIFY channelChanged)

    // Acquisition mode
    Q_PROPERTY(int acquisitionMode READ acquisitionMode
               WRITE setAcquisitionMode NOTIFY acquisitionModeChanged)

    // Simulation per-channel config (CH1)
    Q_PROPERTY(int ch1Waveform READ ch1Waveform WRITE setCh1Waveform
               NOTIFY simulationChanged)
    Q_PROPERTY(double ch1Amplitude READ ch1Amplitude WRITE setCh1Amplitude
               NOTIFY simulationChanged)
    Q_PROPERTY(double ch1Frequency READ ch1Frequency WRITE setCh1Frequency
               NOTIFY simulationChanged)
    Q_PROPERTY(double ch1Phase READ ch1Phase WRITE setCh1Phase
               NOTIFY simulationChanged)
    Q_PROPERTY(double ch1DcOffset READ ch1DcOffset WRITE setCh1DcOffset
               NOTIFY simulationChanged)
    Q_PROPERTY(double ch1DutyCycle READ ch1DutyCycle WRITE setCh1DutyCycle
               NOTIFY simulationChanged)
    Q_PROPERTY(double ch1Noise READ ch1Noise WRITE setCh1Noise
               NOTIFY simulationChanged)

    // Simulation per-channel config (CH2)
    Q_PROPERTY(int ch2Waveform READ ch2Waveform WRITE setCh2Waveform
               NOTIFY simulationChanged)
    Q_PROPERTY(double ch2Amplitude READ ch2Amplitude WRITE setCh2Amplitude
               NOTIFY simulationChanged)
    Q_PROPERTY(double ch2Frequency READ ch2Frequency WRITE setCh2Frequency
               NOTIFY simulationChanged)
    Q_PROPERTY(double ch2Phase READ ch2Phase WRITE setCh2Phase
               NOTIFY simulationChanged)
    Q_PROPERTY(double ch2DcOffset READ ch2DcOffset WRITE setCh2DcOffset
               NOTIFY simulationChanged)
    Q_PROPERTY(double ch2DutyCycle READ ch2DutyCycle WRITE setCh2DutyCycle
               NOTIFY simulationChanged)
    Q_PROPERTY(double ch2Noise READ ch2Noise WRITE setCh2Noise
               NOTIFY simulationChanged)

    // Capability flags (read-only, change with mode)
    Q_PROPERTY(bool supportsWaveformEditor READ supportsWaveformEditor NOTIFY modeChanged)
    Q_PROPERTY(bool supportsForceTrigger READ supportsForceTrigger NOTIFY modeChanged)

public:
    explicit ScopeController(QObject* parent = nullptr);
    ~ScopeController() override;

    // Direct series registration
    Q_INVOKABLE void registerSeries(int channel, QXYSeries* series);
    Q_INVOKABLE void scanDevices();
    Q_INVOKABLE void forceTrigger();
    Q_INVOKABLE QStringList waveformTypes() const;

    // Step lists exposed to QML for display
    Q_INVOKABLE QVariantList timebaseSteps() const;
    Q_INVOKABLE QVariantList voltsPerDivSteps() const;

    // --- Getters ---
    bool testMode() const;
    bool running() const;
    QString statusText() const;

    int triggerSource() const;
    int triggerEdge() const;
    double triggerLevel() const;
    double triggerHoldoff() const;
    double preTriggerPercent() const;
    QString triggerStatus() const;

    int timebaseIndex() const;
    QString timebaseLabel() const;
    double horizontalPosition() const;

    bool ch1Enabled() const;
    int ch1VoltsIndex() const;
    int ch1Coupling() const;
    double ch1Offset() const;
    bool ch2Enabled() const;
    int ch2VoltsIndex() const;
    int ch2Coupling() const;
    double ch2Offset() const;

    int acquisitionMode() const;

    // Simulation config getters
    int ch1Waveform() const;
    double ch1Amplitude() const;
    double ch1Frequency() const;
    double ch1Phase() const;
    double ch1DcOffset() const;
    double ch1DutyCycle() const;
    double ch1Noise() const;

    int ch2Waveform() const;
    double ch2Amplitude() const;
    double ch2Frequency() const;
    double ch2Phase() const;
    double ch2DcOffset() const;
    double ch2DutyCycle() const;
    double ch2Noise() const;

    // Capability flags
    bool supportsWaveformEditor() const;
    bool supportsForceTrigger() const;

    // --- Setters ---
    void setTestMode(bool v);
    void setRunning(bool v);

    void setTriggerSource(int v);
    void setTriggerEdge(int v);
    void setTriggerLevel(double v);
    void setTriggerHoldoff(double v);
    void setPreTriggerPercent(double v);

    void setTimebaseIndex(int v);
    void setHorizontalPosition(double v);

    void setCh1Enabled(bool v);
    void setCh1VoltsIndex(int v);
    void setCh1Coupling(int v);
    void setCh1Offset(double v);
    void setCh2Enabled(bool v);
    void setCh2VoltsIndex(int v);
    void setCh2Coupling(int v);
    void setCh2Offset(double v);

    void setAcquisitionMode(int v);

    // Simulation config setters
    void setCh1Waveform(int v);
    void setCh1Amplitude(double v);
    void setCh1Frequency(double v);
    void setCh1Phase(double v);
    void setCh1DcOffset(double v);
    void setCh1DutyCycle(double v);
    void setCh1Noise(double v);

    void setCh2Waveform(int v);
    void setCh2Amplitude(double v);
    void setCh2Frequency(double v);
    void setCh2Phase(double v);
    void setCh2DcOffset(double v);
    void setCh2DutyCycle(double v);
    void setCh2Noise(double v);

signals:
    void modeChanged();
    void runningChanged();
    void statusChanged();
    void triggerChanged();
    void triggerStatusChanged();
    void timebaseChanged();
    void horizontalChanged();
    void channelChanged();
    void acquisitionModeChanged();
    void simulationChanged();
    void errorOccurred(const QString& msg);

private:
    // Backends
    std::unique_ptr<SignalGenerator> m_signalGen;
    std::unique_ptr<AcquisitionEngine> m_engine;

    // Registered series
    QXYSeries* m_series1 = nullptr;
    QXYSeries* m_series2 = nullptr;

    // State
    bool m_testMode = true;
    bool m_running = false;
    QString m_statusText = "Emulated oscilloscope";
    int m_acquisitionMode = 0; // 0=Auto, 1=Normal, 2=Single

    // Trigger
    int m_triggerSource = 0;
    int m_triggerEdge = 0;
    double m_triggerLevel = 0.0;
    double m_triggerHoldoff = 0.0;
    double m_preTriggerPercent = 25.0;
    QString m_triggerStatus = "Auto";

    // Channel state
    bool m_ch1Enabled = true;
    bool m_ch2Enabled = true;
    int m_ch1Coupling = 1;  // DC
    int m_ch2Coupling = 1;
    double m_ch1Offset = 0.0;
    double m_ch2Offset = 0.0;

    // Standard 1-2-5 step tables
    static const QVector<double> s_timebaseSteps;  // in seconds
    static const QVector<double> s_voltsSteps;      // in volts
    int m_timebaseIndex = 16; // default to 1 ms/div (index 16)
    int m_ch1VoltsIndex = 9;  // default to 1V/div
    int m_ch2VoltsIndex = 9;

    // Horizontal position
    double m_horizontalPosition = 0.0;

    // Simulation config
    ChannelSimConfig m_ch1SimConfig;
    ChannelSimConfig m_ch2SimConfig;

    // Device scanning
    std::vector<DeviceInfo> m_discoveredDevices;

    void updateSeriesFromWaveform(std::shared_ptr<WaveformBuffer> buf);
    void setupSignalGenerator();
    void teardownSignalGenerator();
    void connectToFirstDevice();
};
