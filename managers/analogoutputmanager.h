#ifndef ANALOGOUTPUTMANAGER_H
#define ANALOGOUTPUTMANAGER_H

#include <QObject>
#include <QVector>
#include <QList>
#include <QPointF>
#include <QStringList>
#include <QMutex>
#include <QTimer>
#include <QElapsedTimer>

extern "C" {
#include <NIDAQmx.h>
}

class AnalogOutputManager : public QObject
{
    Q_OBJECT

public:
    enum WaveType {
        Fixed,
        Sine,
        Triangle,
        Square
    };

    explicit AnalogOutputManager(QObject *parent = nullptr);
    ~AnalogOutputManager();

    void setOutputChannels(const QStringList &channels);
    bool outputWaveform(WaveType type, double amplitude);
    void stopOutput();

signals:
    // 每个 tick 发出当前时刻对应的 (time_s, voltage) 点
    void waveformPoint(double timeSec, double voltage);
    void errorOccurred(const QString &error);

private slots:
    void onDisplayTick();

private:
    bool startContinuousAO(const QVector<double> &waveform,
                           const QStringList &channels, double sampleRate);
    void handleDAQmxError(int32 error);

    static const QString AO_DEVICE;
    static const double DEFAULT_FREQUENCY;
    static const double DEFAULT_SAMPLE_RATE;
    static const int DEFAULT_SAMPLES;

    QMutex      m_taskMutex;
    TaskHandle  aoTaskHandle = nullptr;
    QStringList outputChannels;

    QTimer      displayTimer;
    QElapsedTimer elapsedTimer;
    QVector<double> currentWaveform;
    double currentSampleRate = 1000.0;
};

#endif // ANALOGOUTPUTMANAGER_H
