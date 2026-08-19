#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "module/wave_view/mwaveview.h"
#include "gaugewidget.h"
#include "managers/analoginputmanager.h"
#include "managers/analogoutputmanager.h"
#include "managers/digitalio_manager.h"
#include "managers/canmanager.h"
#include "managers/linmanager.h"
#include <QFileDialog>
#include <QTextStream>
#include <QFile>
#include <QTimer>
#include <QMessageBox>
#include <QInputDialog>
#include <QSerialPortInfo>
#include <QRegularExpression>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButton_pause_clicked();
    void saveWaveDataToCSV();
    void startReading();
    void onModeComboBoxChanged(int index);
    void onTabChanged(int index);
    void on_pushButton_dioStart_clicked();
    void on_pushButton_dioStop_clicked();
    void on_radioButton_DO_toggled(bool checked);
    void on_pushButton_canInit_clicked();
    void on_pushButton_canSend_clicked();
    void on_pushButton_linOpen_clicked();
    void on_pushButton_linSend_clicked();
    void on_pushButton_test_clicked();

    void onAIDataReady(int channel, const QList<QPointF> &data);
    void onAOWaveformPoint(double timeSec, double voltage);
    void onDIDataReady(const QVector<bool> &data, const QVector<QPointF> &waveData);
    void onCANFrameReceived(uint32_t id, const QByteArray &data, const QString &timestamp);
    void onLINMasterDataReceived(const QByteArray &data);
    void onManagerError(const QString &error);

private:
    Ui::MainWindow *ui;
    MWaveView *wave;
    GaugeWidget *gauge;

    AnalogInputManager *aiManager;
    AnalogOutputManager *aoManager;
    DigitalIOManager *dioManager;
    CANManager *canManager;
    LINManager *linManager;

    QList<QPointF> wave_data[12];
    bool channelsEnabled[11] = {false};
    int currentMode = 0;  // 0=Input, 1=Output

    QCheckBox *m_aiCheckBoxes[11] = {};   // checkBox_1  ~ checkBox_11  (AI0~AI10)
    QCheckBox *m_dioCheckBoxes[8] = {};   // checkBox_22 ~ checkBox_29  (DO0~DO7)
    QLabel    *m_diLabels[8]      = {};   // label_DI0   ~ label_DI7

    QByteArray hexStringToBytes(const QString &str);
    void updateChannelsEnabled();
    bool selectMasterSlavePort(QString &master, QString &slave);
};

#endif // MAINWINDOW_H
