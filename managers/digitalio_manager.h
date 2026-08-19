#ifndef DIGITALIO_MANAGER_H
#define DIGITALIO_MANAGER_H

#include <QObject>
#include <QTimer>
#include <QThread>
#include <QVector>
#include <QPointF>

extern "C" {
#include <NIDAQmx.h>
}

// Worker lives on a dedicated thread; owns the DAQmx read task and polling timer
class DIOReadWorker : public QObject
{
    Q_OBJECT

public:
    explicit DIOReadWorker(QObject *parent = nullptr);
    ~DIOReadWorker();

public slots:
    void start(const QString &port, int intervalMs);
    void stop();

signals:
    void digitalInputRead(const QVector<bool> &data, const QVector<QPointF> &waveData);
    void errorOccurred(const QString &error);
    void readingStateChanged(bool active);

private slots:
    void readDigitalInput();

private:
    bool rebuildReadTask();
    void clearReadTask();
    void handleDAQmxError(int32 error);

    static const int NUM_DIO_CHANNELS = 8;

    QString currentPort;
    QTimer *diTimer = nullptr;
    int diWaveIndex = 0;
    TaskHandle diTaskHandle = nullptr;
};

class DigitalIOManager : public QObject
{
    Q_OBJECT

public:
    explicit DigitalIOManager(QObject *parent = nullptr);
    ~DigitalIOManager();

    void setPort(const QString &port);
    bool writeDigitalOutput(const QVector<bool> &data);
    void startReadingInput(int intervalMs);
    void stopReadingInput();
    bool isReading() const { return m_reading; }

signals:
    void digitalInputRead(const QVector<bool> &data, const QVector<QPointF> &waveData);
    void errorOccurred(const QString &error);

    // internal signals to worker
    void requestStart(const QString &port, int intervalMs);
    void requestStop();

private:
    static const int NUM_DIO_CHANNELS = 8;

    DIOReadWorker *m_worker;
    QThread       *m_thread;
    bool           m_reading = false;
    QString        m_port;
};

#endif // DIGITALIO_MANAGER_H
