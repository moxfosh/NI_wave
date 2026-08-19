#include "digitalio_manager.h"
#include <QtConcurrent/QtConcurrentRun>

// ── DIOReadWorker ─────────────────────────────────────────────────────────────

DIOReadWorker::DIOReadWorker(QObject *parent)
    : QObject(parent)
{}

DIOReadWorker::~DIOReadWorker()
{
    stop();
}

void DIOReadWorker::start(const QString &port, int intervalMs)
{
    currentPort  = port;
    diWaveIndex  = 0;

    if (!diTimer) {
        diTimer = new QTimer(this);
        connect(diTimer, &QTimer::timeout, this, &DIOReadWorker::readDigitalInput);
    }
    diTimer->stop();
    if (!rebuildReadTask()) return;
    diTimer->start(intervalMs);
    emit readingStateChanged(true);
}

void DIOReadWorker::stop()
{
    if (diTimer) diTimer->stop();
    clearReadTask();
    emit readingStateChanged(false);
}

bool DIOReadWorker::rebuildReadTask()
{
    clearReadTask();
    if (currentPort.isEmpty()) return false;

    int32 error = DAQmxCreateTask("", &diTaskHandle);
    if (DAQmxFailed(error)) { handleDAQmxError(error); diTaskHandle = nullptr; return false; }

    error = DAQmxCreateDIChan(diTaskHandle, currentPort.toStdString().c_str(), "", DAQmx_Val_ChanPerLine);
    if (DAQmxFailed(error)) { handleDAQmxError(error); clearReadTask(); return false; }

    error = DAQmxStartTask(diTaskHandle);
    if (DAQmxFailed(error)) { handleDAQmxError(error); clearReadTask(); return false; }

    return true;
}

void DIOReadWorker::clearReadTask()
{
    if (diTaskHandle) {
        DAQmxStopTask(diTaskHandle);
        DAQmxClearTask(diTaskHandle);
        diTaskHandle = nullptr;
    }
}

void DIOReadWorker::readDigitalInput()
{
    if (!diTaskHandle) return;

    uInt8 data[1][8] = {{0}};
    int32 error = DAQmxReadDigitalLines(diTaskHandle, 1, 1.0,
                                        DAQmx_Val_GroupByChannel, (uInt8*)data, 8,
                                        nullptr, nullptr, nullptr);
    if (DAQmxFailed(error)) {
        handleDAQmxError(error);
        rebuildReadTask();
        return;
    }

    QVector<bool>   inputData(NUM_DIO_CHANNELS);
    QVector<QPointF> waveData(NUM_DIO_CHANNELS);

    for (int i = 0; i < NUM_DIO_CHANNELS; ++i) {
        inputData[i] = (data[0][i] != 0);
        double offset = i * 1.5;
        waveData[i] = QPointF(diWaveIndex, inputData[i] ? (1.0 + offset) : offset);
    }

    diWaveIndex++;
    emit digitalInputRead(inputData, waveData);
}

void DIOReadWorker::handleDAQmxError(int32 error)
{
    char errBuff[2048] = {'\0'};
    DAQmxGetExtendedErrorInfo(errBuff, 2048);
    emit errorOccurred(QString("DAQmx Error: %1").arg(errBuff));
}

// ── DigitalIOManager ──────────────────────────────────────────────────────────

DigitalIOManager::DigitalIOManager(QObject *parent)
    : QObject(parent)
    , m_worker(new DIOReadWorker)
    , m_thread(new QThread(this))
{
    m_worker->moveToThread(m_thread);

    connect(m_worker, &DIOReadWorker::digitalInputRead,  this, &DigitalIOManager::digitalInputRead);
    connect(m_worker, &DIOReadWorker::errorOccurred,     this, &DigitalIOManager::errorOccurred);
    connect(m_worker, &DIOReadWorker::readingStateChanged,
            this, [this](bool active){ m_reading = active; });

    connect(this, &DigitalIOManager::requestStart, m_worker, &DIOReadWorker::start);
    connect(this, &DigitalIOManager::requestStop,  m_worker, &DIOReadWorker::stop);

    connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);

    m_thread->start();
}

DigitalIOManager::~DigitalIOManager()
{
    emit requestStop();
    m_thread->quit();
    m_thread->wait();
}

void DigitalIOManager::setPort(const QString &port)
{
    m_port = port;
}

void DigitalIOManager::startReadingInput(int intervalMs)
{
    emit requestStart(m_port, intervalMs);
}

void DigitalIOManager::stopReadingInput()
{
    emit requestStop();
}

bool DigitalIOManager::writeDigitalOutput(const QVector<bool> &data)
{
    if (m_port.isEmpty() || data.size() != NUM_DIO_CHANNELS) {
        emit errorOccurred("Invalid port or data size");
        return false;
    }

    // Capture by value — runs on thread pool, must not touch 'this'
    QString port = m_port;
    QVector<bool> bits = data;

    QtConcurrent::run([port, bits, this]() {
        uInt8 outputData[1][8] = {{0}};
        for (int i = 0; i < NUM_DIO_CHANNELS; ++i)
            outputData[0][i] = bits[i] ? 1 : 0;

        TaskHandle dioTask = nullptr;
        int32 error = DAQmxCreateTask("", &dioTask);
        if (DAQmxFailed(error)) {
            char errBuff[2048] = {'\0'};
            DAQmxGetExtendedErrorInfo(errBuff, 2048);
            emit errorOccurred(QString("DAQmx Error: %1").arg(errBuff));
            return;
        }

        error = DAQmxCreateDOChan(dioTask, port.toStdString().c_str(), "", DAQmx_Val_ChanPerLine);
        if (DAQmxFailed(error)) {
            char errBuff[2048] = {'\0'};
            DAQmxGetExtendedErrorInfo(errBuff, 2048);
            emit errorOccurred(QString("DAQmx Error: %1").arg(errBuff));
            DAQmxClearTask(dioTask);
            return;
        }

        error = DAQmxStartTask(dioTask);
        if (DAQmxFailed(error)) {
            char errBuff[2048] = {'\0'};
            DAQmxGetExtendedErrorInfo(errBuff, 2048);
            emit errorOccurred(QString("DAQmx Error: %1").arg(errBuff));
            DAQmxClearTask(dioTask);
            return;
        }

        DAQmxWriteDigitalLines(dioTask, 1, 1, 10.0,
                               DAQmx_Val_GroupByChannel, (uInt8*)outputData,
                               nullptr, nullptr);
        DAQmxStopTask(dioTask);
        DAQmxClearTask(dioTask);
    });

    return true;
}
