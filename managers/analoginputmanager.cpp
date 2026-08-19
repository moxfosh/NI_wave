#include "analoginputmanager.h"
#include <vector>

// ── AIWorker ──────────────────────────────────────────────────────────────────

const QString AIWorker::AI_DEVICE      = "PXI1Slot10";
const double  AIWorker::SAMPLE_RATE    = 10000.0;

AIWorker::AIWorker(QObject *parent)
    : QObject(parent)
    , channelsEnabled(NUM_CHANNELS, false)
{}

AIWorker::~AIWorker()
{
    stop();
}

void AIWorker::setChannels(const QVector<bool> &enabled)
{
    if (enabled.size() < NUM_CHANNELS) return;
    for (int i = 0; i < NUM_CHANNELS; ++i)
        channelsEnabled[i] = enabled[i];
}

void AIWorker::start(int intervalMs)
{
    if (!sampleTimer) {
        sampleTimer = new QTimer(this);
        connect(sampleTimer, &QTimer::timeout, this, &AIWorker::sampleData);
    }
    sampleTimer->stop();
    if (!rebuildTask()) return;
    sampleTimer->start(intervalMs);
    emit samplingStateChanged(true);
}

void AIWorker::stop()
{
    if (sampleTimer) sampleTimer->stop();
    clearTask();
    emit samplingStateChanged(false);
}

bool AIWorker::rebuildTask()
{
    clearTask();

    QStringList enabledChannels;
    for (int i = 0; i < NUM_CHANNELS; ++i) {
        if (channelsEnabled[i])
            enabledChannels << QString("%1/ai%2").arg(AI_DEVICE).arg(i);
    }
    if (enabledChannels.isEmpty()) return false;

    int32 error = DAQmxCreateTask("", &taskHandle);
    if (DAQmxFailed(error)) { handleDAQmxError(error); taskHandle = nullptr; return false; }

    QString chanStr = enabledChannels.join(",");
    error = DAQmxCreateAIVoltageChan(taskHandle, chanStr.toStdString().c_str(), "",
                                     DAQmx_Val_RSE, -10.0, 10.0, DAQmx_Val_Volts, NULL);
    if (DAQmxFailed(error)) { handleDAQmxError(error); clearTask(); return false; }

    error = DAQmxCfgSampClkTiming(taskHandle, "", SAMPLE_RATE,
                                  DAQmx_Val_Rising, DAQmx_Val_ContSamps,
                                  SAMPLES_PER_CHANNEL);
    if (DAQmxFailed(error)) { handleDAQmxError(error); clearTask(); return false; }

    error = DAQmxStartTask(taskHandle);
    if (DAQmxFailed(error)) { handleDAQmxError(error); clearTask(); return false; }

    activeChannels = enabledChannels;
    return true;
}

void AIWorker::clearTask()
{
    if (taskHandle) {
        DAQmxStopTask(taskHandle);
        DAQmxClearTask(taskHandle);
        taskHandle = nullptr;
    }
    activeChannels.clear();
}

void AIWorker::sampleData()
{
    if (!taskHandle) return;

    int activeChannelCount = activeChannels.size();
    int totalSamples = SAMPLES_PER_CHANNEL * activeChannelCount;
    std::vector<double> data(totalSamples, 0.0);
    int32 samplesRead = 0;

    int32 error = DAQmxReadAnalogF64(taskHandle, SAMPLES_PER_CHANNEL, 1.0,
                                     DAQmx_Val_GroupByScanNumber,
                                     data.data(), totalSamples, &samplesRead, NULL);
    if (DAQmxFailed(error)) {
        handleDAQmxError(error);
        rebuildTask();
        return;
    }

    for (int ch = 0; ch < activeChannelCount; ++ch) {
        QList<QPointF> points;
        points.reserve(samplesRead);
        for (int i = 0; i < samplesRead; ++i)
            points.append(QPointF(i, data[i * activeChannelCount + ch]));

        const QString &chName = activeChannels[ch];
        int logicalIndex = chName.mid(chName.indexOf("ai") + 2).toInt();
        emit dataReady(logicalIndex, points);
    }
}

void AIWorker::handleDAQmxError(int32 error)
{
    char errBuff[2048] = {'\0'};
    DAQmxGetExtendedErrorInfo(errBuff, 2048);
    emit errorOccurred(QString("DAQmx Error: %1").arg(errBuff));
}

// ── AnalogInputManager ────────────────────────────────────────────────────────

AnalogInputManager::AnalogInputManager(QObject *parent)
    : QObject(parent)
    , m_worker(new AIWorker)
    , m_thread(new QThread(this))
{
    m_worker->moveToThread(m_thread);

    // worker → manager (cross-thread, auto-queued)
    connect(m_worker, &AIWorker::dataReady,          this, &AnalogInputManager::dataReady);
    connect(m_worker, &AIWorker::errorOccurred,      this, &AnalogInputManager::errorOccurred);
    connect(m_worker, &AIWorker::samplingStateChanged,
            this, [this](bool active){ m_sampling = active; });

    // manager → worker commands (queued across thread boundary)
    connect(this, &AnalogInputManager::requestStart,       m_worker, &AIWorker::start);
    connect(this, &AnalogInputManager::requestStop,        m_worker, &AIWorker::stop);
    connect(this, &AnalogInputManager::requestSetChannels, m_worker, &AIWorker::setChannels);

    // clean up worker when thread finishes
    connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);

    m_thread->start();
}

AnalogInputManager::~AnalogInputManager()
{
    emit requestStop();
    m_thread->quit();
    m_thread->wait();
}

void AnalogInputManager::setChannelsEnabled(const QVector<bool> &enabled)
{
    emit requestSetChannels(enabled);
}

void AnalogInputManager::startSampling(int intervalMs)
{
    emit requestStart(intervalMs);
}

void AnalogInputManager::stopSampling()
{
    emit requestStop();
}
