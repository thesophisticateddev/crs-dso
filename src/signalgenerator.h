#ifndef SIGNALGENERATOR_H
#define SIGNALGENERATOR_H

#include <QObject>
#include <QTimer>
#include <QThread>
#include <QPointF>
#include <QList>
#include <QtCharts/QXYSeries>
#include <cmath>
#include <random>

// --- Waveform types ---
enum class WaveformType {
    Sine = 0,
    Square,
    Triangle,
    Sawtooth,
    Pulse,
    DC,
    Noise
};

// --- Per-channel simulation configuration ---
struct ChannelSimConfig {
    WaveformType waveform = WaveformType::Sine;
    double amplitude = 1.0;    // volts peak
    double frequency = 1000.0; // Hz
    double phase = 0.0;        // radians
    double dcOffset = 0.0;     // volts
    double dutyCycle = 0.5;    // 0-1, for pulse/square
    double noise = 0.0;        // noise amplitude in volts
};

// --- The Worker: Runs in the background thread ---
class SignalWorker : public QObject {
    Q_OBJECT
public:
    SignalWorker() = default;

public slots:
    void generateData();

    // Trigger
    void setTriggerLevel(double l) { m_triggerLevel = l; }
    void setTriggerSource(int ch) { m_triggerSource = ch; }
    void setTriggerEdge(int edge) { m_triggerEdge = edge; }
    void setTriggerHoldoff(double s) { m_triggerHoldoff = s; }
    void setPreTriggerPercent(double p) { m_preTriggerPercent = p; }

    // Channels
    void setChannels(bool c1, bool c2) { m_ch1Active = c1; m_ch2Active = c2; }
    void setRunning(bool running) { m_running = running; }
    void setCh1Offset(double v) { m_ch1Offset = v; }
    void setCh2Offset(double v) { m_ch2Offset = v; }

    // Acquisition
    void setAcquisitionMode(int mode);
    void forceTrigger() { m_forceTrigger = true; }

    // Timebase & sampling
    void setSampleRate(double rate) { m_sampleRate = rate; }
    void setTimebaseSeconds(double s) { m_timebaseSeconds = s; }
    void setHorizontalPosition(double divs) { m_horizontalPosition = divs; }

    // Per-channel simulation config
    void setCh1Waveform(int w) { m_ch1Config.waveform = static_cast<WaveformType>(w); }
    void setCh1Amplitude(double v) { m_ch1Config.amplitude = v; }
    void setCh1Frequency(double v) { m_ch1Config.frequency = v; }
    void setCh1Phase(double v) { m_ch1Config.phase = v; }
    void setCh1DcOffset(double v) { m_ch1Config.dcOffset = v; }
    void setCh1DutyCycle(double v) { m_ch1Config.dutyCycle = v; }
    void setCh1Noise(double v) { m_ch1Config.noise = v; }

    void setCh2Waveform(int w) { m_ch2Config.waveform = static_cast<WaveformType>(w); }
    void setCh2Amplitude(double v) { m_ch2Config.amplitude = v; }
    void setCh2Frequency(double v) { m_ch2Config.frequency = v; }
    void setCh2Phase(double v) { m_ch2Config.phase = v; }
    void setCh2DcOffset(double v) { m_ch2Config.dcOffset = v; }
    void setCh2DutyCycle(double v) { m_ch2Config.dutyCycle = v; }
    void setCh2Noise(double v) { m_ch2Config.noise = v; }

signals:
    void dataReady(int channel, const QList<QPointF> &points);
    void triggerStatusChanged(const QString &status);

private:
    double generateSample(const ChannelSimConfig &cfg, double t);
    int findTriggerPoint(const std::vector<double> &samples, int numSamples);
    double gaussianRandom();

    // Per-channel config
    ChannelSimConfig m_ch1Config;
    ChannelSimConfig m_ch2Config;

    // Trigger
    double m_triggerLevel = 0.0;
    int m_triggerSource = 0;
    int m_triggerEdge = 0;
    double m_triggerHoldoff = 0.0;
    double m_preTriggerPercent = 25.0;

    // Sampling
    double m_sampleRate = 1e6;
    double m_timebaseSeconds = 1e-3;
    double m_horizontalPosition = 0.0;

    // Channel state
    double m_ch1Offset = 0.0;
    double m_ch2Offset = 0.0;
    bool m_ch1Active = false;
    bool m_ch2Active = false;

    // Acquisition
    bool m_running = true;
    int m_acquisitionMode = 0; // 0=Auto, 1=Normal, 2=Single
    bool m_singleShotFired = false;
    bool m_forceTrigger = false;

    // Time tracking
    double m_timeOffset = 0.0;

    // RNG
    std::mt19937 m_rng{std::random_device{}()};
    std::normal_distribution<double> m_normalDist{0.0, 1.0};
};

// --- The Generator: Stays in the UI thread for QML ---
class SignalGenerator : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY(int triggerSource READ triggerSource WRITE setTriggerSource NOTIFY triggerSourceChanged)
    Q_PROPERTY(int triggerEdge READ triggerEdge WRITE setTriggerEdge NOTIFY triggerEdgeChanged)
    Q_PROPERTY(double triggerLevel READ triggerLevel WRITE setTriggerLevel NOTIFY triggerLevelChanged)
    Q_PROPERTY(QString triggerStatus READ triggerStatus NOTIFY triggerStatusChanged)

public:
    explicit SignalGenerator(QObject *parent = nullptr);
    ~SignalGenerator();

    // Property getters
    bool running() const { return m_running; }
    int triggerSource() const { return m_triggerSource; }
    int triggerEdge() const { return m_triggerEdge; }
    double triggerLevel() const { return m_triggerLevel; }
    QString triggerStatus() const { return m_triggerStatus; }

    // QML-facing methods
    Q_INVOKABLE void setTriggerLevel(double level);
    Q_INVOKABLE void setTriggerSource(int channel);
    Q_INVOKABLE void setTriggerEdge(int edge);
    Q_INVOKABLE void setRunning(bool running);
    Q_INVOKABLE void registerSeries(int channel, QXYSeries *series);

    // Trigger expansion
    void setTriggerHoldoff(double s);
    void setPreTriggerPercent(double p);
    void forceTrigger();

    // Acquisition
    void setAcquisitionMode(int mode);

    // Timebase & sampling
    void setSampleRate(double rate);
    void setTimebaseSeconds(double s);
    void setHorizontalPosition(double divs);

    // Channel offsets
    void setCh1Offset(double v);
    void setCh2Offset(double v);

    // Per-channel simulation config
    void setCh1Waveform(int w);
    void setCh1Amplitude(double v);
    void setCh1Frequency(double v);
    void setCh1Phase(double v);
    void setCh1DcOffset(double v);
    void setCh1DutyCycle(double v);
    void setCh1Noise(double v);

    void setCh2Waveform(int w);
    void setCh2Amplitude(double v);
    void setCh2Frequency(double v);
    void setCh2Phase(double v);
    void setCh2DcOffset(double v);
    void setCh2DutyCycle(double v);
    void setCh2Noise(double v);

signals:
    void runningChanged();
    void triggerSourceChanged();
    void triggerEdgeChanged();
    void triggerLevelChanged();
    void triggerStatusChanged(const QString &status);

private slots:
    void handleDataReady(int channel, const QList<QPointF> &points);
    void handleTriggerStatus(const QString &status);

private:
    QThread m_workerThread;
    SignalWorker *m_worker;
    QXYSeries *m_series1 = nullptr;
    QXYSeries *m_series2 = nullptr;

    bool m_running = true;
    int m_triggerSource = 0;
    int m_triggerEdge = 0;
    double m_triggerLevel = 0.0;
    QString m_triggerStatus = "Auto";
};

#endif
