#ifndef SIGNALGENERATOR_H
#define SIGNALGENERATOR_H

#include <QObject>
#include <QTimer>
#include <QThread>
#include <QPointF>
#include <QList>
#include <QtCharts/QXYSeries>
#include <cmath>

// --- The Worker: Runs in the background thread ---
class SignalWorker : public QObject {
    Q_OBJECT
public:
    SignalWorker() = default;

public slots:
    void generateData();
    void setTriggerLevel(double l) { m_triggerLevel = l; }
    void setTriggerSource(int ch) { m_triggerSource = ch; }
    void setTriggerEdge(int edge) { m_triggerEdge = edge; } // 0=rising, 1=falling
    void setChannels(bool c1, bool c2) { m_ch1Active = c1; m_ch2Active = c2; }
    void setRunning(bool running) { m_running = running; }

signals:
    void dataReady(int channel, const QList<QPointF> &points);

private:
    double generateCh1Sample(int idx);
    double generateCh2Sample(int idx);
    double generateSample(int channel, int idx);
    int findTriggerPoint(int bufferSize, int displayPoints);

    double m_index = 0;
    double m_triggerLevel = 1.5;
    int m_triggerSource = 0;  // 0=CH1, 1=CH2
    int m_triggerEdge = 0;    // 0=rising, 1=falling
    bool m_ch1Active = false;
    bool m_ch2Active = false;
    bool m_running = true;
};

// --- The Generator: Stays in the UI thread for QML ---
class SignalGenerator : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY(int triggerSource READ triggerSource WRITE setTriggerSource NOTIFY triggerSourceChanged)
    Q_PROPERTY(int triggerEdge READ triggerEdge WRITE setTriggerEdge NOTIFY triggerEdgeChanged)
    Q_PROPERTY(double triggerLevel READ triggerLevel WRITE setTriggerLevel NOTIFY triggerLevelChanged)

public:
    explicit SignalGenerator(QObject *parent = nullptr);
    ~SignalGenerator();

    // Property getters
    bool running() const { return m_running; }
    int triggerSource() const { return m_triggerSource; }
    int triggerEdge() const { return m_triggerEdge; }
    double triggerLevel() const { return m_triggerLevel; }

    // QML-facing methods
    Q_INVOKABLE void setTriggerLevel(double level);
    Q_INVOKABLE void setTriggerSource(int channel);
    Q_INVOKABLE void setTriggerEdge(int edge);
    Q_INVOKABLE void setRunning(bool running);
    Q_INVOKABLE void registerSeries(int channel, QXYSeries *series);

signals:
    void runningChanged();
    void triggerSourceChanged();
    void triggerEdgeChanged();
    void triggerLevelChanged();

private slots:
    void handleDataReady(int channel, const QList<QPointF> &points);

private:
    QThread m_workerThread;
    SignalWorker *m_worker;
    QXYSeries *m_series1 = nullptr;
    QXYSeries *m_series2 = nullptr;
    bool m_running = true;
    int m_triggerSource = 0;
    int m_triggerEdge = 0;
    double m_triggerLevel = 1.5;
};

#endif
