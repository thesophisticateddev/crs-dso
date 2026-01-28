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
    void setChannels(bool c1, bool c2) { m_ch1Active = c1; m_ch2Active = c2; }

signals:
    void dataReady(int channel, const QList<QPointF> &points);

private:
    double m_index = 0;
    double m_triggerLevel = 1.5;
    bool m_ch1Active = false;
    bool m_ch2Active = false;
};

// --- The Generator: Stays in the UI thread for QML ---
class SignalGenerator : public QObject {
    Q_OBJECT
public:
    explicit SignalGenerator(QObject *parent = nullptr);
    ~SignalGenerator();

    // QML-facing methods
    Q_INVOKABLE void setTriggerLevel(double level);
    Q_INVOKABLE void registerSeries(int channel, QXYSeries *series);

private slots:
    void handleDataReady(int channel, const QList<QPointF> &points);

private:
    QThread m_workerThread;
    SignalWorker *m_worker;
    QXYSeries *m_series1 = nullptr;
    QXYSeries *m_series2 = nullptr;
};

#endif
