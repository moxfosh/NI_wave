#include "waveformgenerator.h"

QVector<double> WaveformGenerator::generateFixed(double value, int samples)
{
    return QVector<double>(samples, value);
}

QVector<double> WaveformGenerator::generateSine(double amplitude, double frequency,
                                                double samplingRate, int samples)
{
    QVector<double> waveform;
    waveform.reserve(samples);

    for (int i = 0; i < samples; ++i) {
        double t = i / samplingRate;
        waveform.append(amplitude * qSin(2 * M_PI * frequency * t));
    }

    return waveform;
}

QVector<double> WaveformGenerator::generateTriangle(double amplitude, double frequency,
                                                    double samplingRate, int samples)
{
    QVector<double> waveform;
    waveform.reserve(samples);

    double period = 1.0 / frequency;

    for (int i = 0; i < samples; ++i) {
        double t = i / samplingRate;
        double val = amplitude * (2 * qFabs(2 * ((t / period) - qFloor(t / period + 0.5))) - 1);
        waveform.append(val);
    }

    return waveform;
}

QVector<double> WaveformGenerator::generateSquare(double amplitude, double frequency,
                                                  double samplingRate, int samples)
{
    QVector<double> waveform;
    waveform.reserve(samples);

    double period = 1.0 / frequency;

    for (int i = 0; i < samples; ++i) {
        double t = i / samplingRate;
        double value = (fmod(t, period) < period / 2) ? amplitude : -amplitude;
        waveform.append(value);
    }

    return waveform;
}
