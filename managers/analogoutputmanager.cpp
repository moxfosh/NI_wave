#include "analogoutputmanager.h"
#include "../utils/waveformgenerator.h"

const QString AnalogOutputManager::AO_DEVICE          = "PXI1Slot10";
const double  AnalogOutputManager::DEFAULT_FREQUENCY   = 50.0;
const double  AnalogOutputManager::DEFAULT_SAMPLE_RATE = 1000.0;
const int     AnalogOutputManager::DEFAULT_SAMPLES     = 1000;

AnalogOutputManager::AnalogOutputManager(QObject *parent)
    : QObject(parent)
    , aoTaskHandle(nullptr)
{
    connect(&displayTimer, &QTimer::timeout, this, &AnalogOutputManager::onDisplayTick);
}

AnalogOutputManager::~AnalogOutputManager()
{
    stopOutput();
}

void AnalogOutputManager::setOutputChannels(const QStringList &channels)
{
    outputChannels = channels;
}

bool AnalogOutputManager::outputWaveform(WaveType type, double amplitude)
{
    QVector<double> waveform;

    switch (type) {
    case Fixed:
        waveform = WaveformGenerator::generateFixed(amplitude, DEFAULT_SAMPLES);
        break;
    case Sine:
        waveform = WaveformGenerator::generateSine(amplitude, DEFAULT_FREQUENCY,
                                                   DEFAULT_SAMPLE_RATE, DEFAULT_SAMPLES);
        break;
    case Triangle:
        waveform = WaveformGenerator::generateTriangle(amplitude, DEFAULT_FREQUENCY,
                                                       DEFAULT_SAMPLE_RATE, DEFAULT_SAMPLES);
        break;
    case Square:
        waveform = WaveformGenerator::generateSquare(amplitude, DEFAULT_FREQUENCY,
                                                     DEFAULT_SAMPLE_RATE, DEFAULT_SAMPLES);
        break;
    default:
        return false;
    }

    if (outputChannels.isEmpty()) return false;

    if (!startContinuousAO(waveform, outputChannels, DEFAULT_SAMPLE_RATE))
        return false;

    currentWaveform   = waveform;
    currentSampleRate = DEFAULT_SAMPLE_RATE;

    elapsedTimer.restart();
    displayTimer.start(20);  // 50Hz 刷新显示
    return true;
}

void AnalogOutputManager::onDisplayTick()
{
    if (currentWaveform.isEmpty()) return;

    double timeSec = elapsedTimer.elapsed() / 1000.0;

    int period = currentWaveform.size();
    double samplesElapsed = timeSec * currentSampleRate;
    int idx = static_cast<int>(samplesElapsed) % period;

    emit waveformPoint(timeSec, currentWaveform[idx]);
}

void AnalogOutputManager::stopOutput()
{
    displayTimer.stop();
    currentWaveform.clear();

    QMutexLocker locker(&m_taskMutex);
    if (aoTaskHandle) {
        DAQmxStopTask(aoTaskHandle);
        DAQmxClearTask(aoTaskHandle);
        aoTaskHandle = nullptr;
    }
}

bool AnalogOutputManager::startContinuousAO(const QVector<double> &waveform,
                                             const QStringList &channels,
                                             double sampleRate)
{
    {
        QMutexLocker locker(&m_taskMutex);
        if (aoTaskHandle) {
            DAQmxStopTask(aoTaskHandle);
            DAQmxClearTask(aoTaskHandle);
            aoTaskHandle = nullptr;
        }
    }

    int samples     = waveform.size();
    int nCh         = channels.size();
    QString chanStr = channels.join(",");

    TaskHandle newHandle = nullptr;

    int32 error = DAQmxCreateTask("", &newHandle);
    if (DAQmxFailed(error)) { handleDAQmxError(error); return false; }

    error = DAQmxCreateAOVoltageChan(newHandle, chanStr.toStdString().c_str(), "",
                                     -10.0, 10.0, DAQmx_Val_Volts, nullptr);
    if (DAQmxFailed(error)) { handleDAQmxError(error); DAQmxClearTask(newHandle); return false; }

    error = DAQmxCfgSampClkTiming(newHandle, "", sampleRate, DAQmx_Val_Rising,
                                  DAQmx_Val_ContSamps, samples);
    if (DAQmxFailed(error)) { handleDAQmxError(error); DAQmxClearTask(newHandle); return false; }

    QVector<double> interleavedData(samples * nCh);
    for (int ch = 0; ch < nCh; ++ch)
        for (int i = 0; i < samples; ++i)
            interleavedData[ch * samples + i] = waveform[i];

    error = DAQmxWriteAnalogF64(newHandle, samples, false, 10.0,
                                DAQmx_Val_GroupByChannel,
                                interleavedData.data(), nullptr, nullptr);
    if (DAQmxFailed(error)) { handleDAQmxError(error); DAQmxClearTask(newHandle); return false; }

    error = DAQmxStartTask(newHandle);
    if (DAQmxFailed(error)) { handleDAQmxError(error); DAQmxClearTask(newHandle); return false; }

    QMutexLocker locker(&m_taskMutex);
    aoTaskHandle = newHandle;
    return true;
}

void AnalogOutputManager::handleDAQmxError(int32 error)
{
    char errBuff[2048] = {'\0'};
    DAQmxGetExtendedErrorInfo(errBuff, 2048);
    emit errorOccurred(QString("DAQmx Error: %1").arg(errBuff));
}
