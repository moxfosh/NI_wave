#ifndef WAVEFORMGENERATOR_H
#define WAVEFORMGENERATOR_H

#include <QVector>
#include <QtMath>

class WaveformGenerator
{
public:
    static QVector<double> generateFixed(double value, int samples);

    static QVector<double> generateSine(double amplitude, double frequency,
                                        double samplingRate, int samples);

    static QVector<double> generateTriangle(double amplitude, double frequency,
                                            double samplingRate, int samples);

    static QVector<double> generateSquare(double amplitude, double frequency,
                                          double samplingRate, int samples);
};

#endif // WAVEFORMGENERATOR_H
