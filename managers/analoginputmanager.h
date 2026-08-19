#ifndef ANALOGINPUTMANAGER_H
#define ANALOGINPUTMANAGER_H

#include <QObject>
#include <QTimer>
#include <QThread>
#include <QList>
#include <QPointF>
#include <QVector>
#include <QMutex>

extern "C" {
#include <NIDAQmx.h>
}

// Worker lives on a dedicated thread; owns the DAQmx task and polling timer
class AIWorker : public QObject
{
    Q_OBJECT

public:
    explicit AIWorker(QObject *parent = nullptr);
    ~AIWorker();

public slots:
    void start(int intervalMs);
    void stop();
    void setChannels(const QVector<bool> &enabled);

signals:
    void dataReady(int channel, const QList<QPointF> &data);
    void errorOccurred(const QString &error);
    void samplingStateChanged(bool active);

private slots:
    void sampleData();

private:
    bool rebuildTask();
    void clearTask();
    void handleDAQmxError(int32 error);

    static const QString AI_DEVICE;
    static const int NUM_CHANNELS = 11;
    static const int SAMPLES_PER_CHANNEL = 1000;
    static const double SAMPLE_RATE;

    QTimer *sampleTimer = nullptr;
    QVector<bool> channelsEnabled;
    TaskHandle taskHandle = nullptr;
    QStringList activeChannels;
};

class AnalogInputManager : public QObject
{
    Q_OBJECT

public:
    explicit AnalogInputManager(QObject *parent = nullptr);
    ~AnalogInputManager();

    void setChannelsEnabled(const QVector<bool> &enabled);
    void startSampling(int intervalMs);
    void stopSampling();
    bool isSampling() const { return m_sampling; }

signals:
    void dataReady(int channel, const QList<QPointF> &data);
    void errorOccurred(const QString &error);

    // internal signals to worker (queued across thread boundary)
    void requestStart(int intervalMs);
    void requestStop();
    void requestSetChannels(const QVector<bool> &enabled);

private:
    AIWorker  *m_worker;
    QThread   *m_thread;
    bool       m_sampling = false;
};

#endif // ANALOGINPUTMANAGER_H
