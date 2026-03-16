#pragma once
#include <QObject>
#include <QVariantList>
#include <QtCharts/QXYSeries>
#include <memory>

class SignalGenerator;
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

    // Timebase (using step index, not raw value)
    Q_PROPERTY(int timebaseIndex READ timebaseIndex WRITE setTimebaseIndex
               NOTIFY timebaseChanged)
    Q_PROPERTY(QString timebaseLabel READ timebaseLabel NOTIFY timebaseChanged)

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

public:
    explicit ScopeController(QObject* parent = nullptr);
    ~ScopeController() override;

    // Direct series registration — avoids QVariantList entirely
    Q_INVOKABLE void registerSeries(int channel, QXYSeries* series);
    Q_INVOKABLE void scanDevices();

    // Step lists exposed to QML for display
    Q_INVOKABLE QVariantList timebaseSteps() const;
    Q_INVOKABLE QVariantList voltsPerDivSteps() const;

    // Getters
    bool testMode() const;
    bool running() const;
    QString statusText() const;
    int triggerSource() const;
    int triggerEdge() const;
    double triggerLevel() const;
    int timebaseIndex() const;
    QString timebaseLabel() const;
    bool ch1Enabled() const;
    int ch1VoltsIndex() const;
    int ch1Coupling() const;
    double ch1Offset() const;
    bool ch2Enabled() const;
    int ch2VoltsIndex() const;
    int ch2Coupling() const;
    double ch2Offset() const;
    int acquisitionMode() const;

    // Setters
    void setTestMode(bool v);
    void setRunning(bool v);
    void setTriggerSource(int v);
    void setTriggerEdge(int v);
    void setTriggerLevel(double v);
    void setTimebaseIndex(int v);
    void setCh1Enabled(bool v);
    void setCh1VoltsIndex(int v);
    void setCh1Coupling(int v);
    void setCh1Offset(double v);
    void setCh2Enabled(bool v);
    void setCh2VoltsIndex(int v);
    void setCh2Coupling(int v);
    void setCh2Offset(double v);
    void setAcquisitionMode(int v);

signals:
    void modeChanged();
    void runningChanged();
    void statusChanged();
    void triggerChanged();
    void timebaseChanged();
    void channelChanged();
    void acquisitionModeChanged();
    void errorOccurred(const QString& msg);

private:
    // Backends
    std::unique_ptr<SignalGenerator> m_signalGen;
    std::unique_ptr<AcquisitionEngine> m_engine;

    // Registered series — updated directly, no QVariantList
    QXYSeries* m_series1 = nullptr;
    QXYSeries* m_series2 = nullptr;

    // State
    bool m_testMode = true;
    bool m_running = false;
    QString m_statusText = "Emulated oscilloscope";
    int m_acquisitionMode = 0; // 0=Auto, 1=Normal, 2=Single

    // Trigger
    int m_triggerSource = 0;  // 0=CH1, 1=CH2
    int m_triggerEdge = 0;    // 0=Rising, 1=Falling
    double m_triggerLevel = 0.0;

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
    int m_timebaseIndex = 12; // default to 1ms/div
    int m_ch1VoltsIndex = 9;  // default to 1V/div
    int m_ch2VoltsIndex = 9;

    // Device scanning
    std::vector<DeviceInfo> m_discoveredDevices;

    void updateSeriesFromWaveform(std::shared_ptr<WaveformBuffer> buf);
    void setupSignalGenerator();
    void teardownSignalGenerator();
    void connectToFirstDevice();
};
