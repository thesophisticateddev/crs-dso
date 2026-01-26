#ifndef SIGNALGENERATOR_H
#define SIGNALGENERATOR_H

#include <QObject>
#include <QTimer>
#include <QtCharts/QXYSeries>
#include <cmath>

class SignalGenerator : public QObject
{
    Q_OBJECT
public:
    explicit SignalGenerator(QObject *parent = nullptr);

    // Updated to accept a channel index (0 or 1)
    Q_INVOKABLE void updateSeries(int channel, QAbstractSeries *series);

private slots:
    void generateData();

private:
    QTimer *m_timer;
    QXYSeries *m_series1 = nullptr; // Channel 1
    QXYSeries *m_series2 = nullptr; // Channel 2
    double m_index = 0;
    const int m_maxPoints = 2000;
};

#endif
