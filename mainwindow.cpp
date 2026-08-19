#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QVBoxLayout>
#include <QDir>
#include <QCheckBox>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <cstdint>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    wave = new MWaveView(this);
    QVBoxLayout *layout = new QVBoxLayout(ui->widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(wave);

    gauge = new GaugeWidget(this);
    gauge->move(1020, 390);
    gauge->hide();

    aiManager = new AnalogInputManager(this);
    aoManager = new AnalogOutputManager(this);
    dioManager = new DigitalIOManager(this);
    canManager = new CANManager(this);
    linManager = new LINManager(this);

    // 缓存控件指针，避免在循环中重复 findChild
    for (int i = 0; i < 11; ++i)
        m_aiCheckBoxes[i] = findChild<QCheckBox*>(QString("checkBox_%1").arg(i + 1));
    for (int i = 0; i < 8; ++i) {
        m_dioCheckBoxes[i] = findChild<QCheckBox*>(QString("checkBox_%1").arg(i + 22));
        m_diLabels[i]      = findChild<QLabel*>(QString("label_DI%1").arg(i));
    }

    connect(aiManager, &AnalogInputManager::dataReady, this, &MainWindow::onAIDataReady);
    connect(aiManager, &AnalogInputManager::errorOccurred, this, &MainWindow::onManagerError);

    connect(aoManager, &AnalogOutputManager::waveformPoint, this, &MainWindow::onAOWaveformPoint);
    connect(aoManager, &AnalogOutputManager::errorOccurred, this, &MainWindow::onManagerError);

    connect(dioManager, &DigitalIOManager::digitalInputRead, this, &MainWindow::onDIDataReady);
    connect(dioManager, &DigitalIOManager::errorOccurred, this, &MainWindow::onManagerError);

    connect(canManager, &CANManager::frameReceived, this, &MainWindow::onCANFrameReceived);
    connect(canManager, &CANManager::errorOccurred, this, &MainWindow::onManagerError);

    connect(linManager, &LINManager::masterDataReceived, this, &MainWindow::onLINMasterDataReceived);
    connect(linManager, &LINManager::errorOccurred, this, &MainWindow::onManagerError);

    connect(ui->pushButton_start, &QPushButton::clicked, this, &MainWindow::startReading);
    connect(ui->pushButton_save, &QPushButton::clicked, this, &MainWindow::saveWaveDataToCSV);
    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);
    connect(ui->comboBox_choosemodel, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onModeComboBoxChanged);
    connect(ui->pushButton_linTest, &QPushButton::clicked, this, &MainWindow::on_pushButton_test_clicked);

    ui->spinBox_interval->setRange(1, 1000);
    ui->spinBox_interval->setSuffix(" ms");
    ui->spinBox_interval->setValue(100);

    ui->doubleSpinBox_output->setRange(-10.0, 10.0);
    ui->doubleSpinBox_output->setSingleStep(0.1);
    ui->doubleSpinBox_output->setValue(0.0);

    currentMode = ui->comboBox_choosemodel->currentIndex();
    onModeComboBoxChanged(currentMode);

    QTimer::singleShot(0, this, [this]() {
        onTabChanged(ui->tabWidget->currentIndex());
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::startReading()
{
    updateChannelsEnabled();

    for (int i = 0; i <= WAVE_CH10; ++i) {
        wave->clearChannel((WAVE_CH)i);
        wave_data[i].clear();
    }
    wave->clearChannel(WAVE_CH11);

    int currentTab = ui->tabWidget->currentIndex();
    if (currentTab == 2) {
        for (int i = WAVE_CH12; i <= WAVE_CH19; ++i) {
            wave->clearChannel((WAVE_CH)i);
            wave->openChannel((WAVE_CH)i);
        }
    } else {
        for (int i = WAVE_CH12; i <= WAVE_CH19; ++i) {
            wave->closeChannel((WAVE_CH)i);
        }
    }

    if (currentMode == 0) {
        QVector<bool> enabled(11);
        for (int i = 0; i < 11; ++i) {
            enabled[i] = channelsEnabled[i];
        }
        aiManager->setChannelsEnabled(enabled);

        int intervalMs = ui->spinBox_interval->value();
        aiManager->startSampling(intervalMs);
    } else {
        double amplitude = ui->doubleSpinBox_output->value();

        QStringList outputChannels;
        outputChannels << "PXI1Slot10/ao0" << "PXI1Slot10/ao1";
        aoManager->setOutputChannels(outputChannels);

        AnalogOutputManager::WaveType type;
        if (ui->radioButton->isChecked()) {
            type = AnalogOutputManager::Fixed;
        } else if (ui->radioButton_2->isChecked()) {
            type = AnalogOutputManager::Sine;
        } else if (ui->radioButton_3->isChecked()) {
            type = AnalogOutputManager::Triangle;
        } else if (ui->radioButton_4->isChecked()) {
            type = AnalogOutputManager::Square;
        } else {
            return;
        }

        if (!aoManager->outputWaveform(type, amplitude)) {
            QMessageBox::warning(this, "提示", "输出失败");
            return;
        }
    }

    wave->startGraph();
}

void MainWindow::updateChannelsEnabled()
{
    for (int i = 0; i < 11; ++i)
        channelsEnabled[i] = m_aiCheckBoxes[i] && m_aiCheckBoxes[i]->isChecked();
}

void MainWindow::onAIDataReady(int channel, const QList<QPointF> &data)
{
    wave->addSeriesData((WAVE_CH)channel, data);
    wave_data[channel] = data;

    if (channel == 0 && !data.isEmpty()) {
        gauge->setVoltage(data.last().y());
    }

    // 最后一个启用通道更新完后统一刷新坐标轴，避免每通道触发一次重绘
    int lastEnabled = -1;
    for (int i = 0; i < 11; ++i)
        if (channelsEnabled[i]) lastEnabled = i;
    if (channel == lastEnabled)
        wave->flushRange();
}

void MainWindow::saveWaveDataToCSV()
{
    QString filePath = QFileDialog::getSaveFileName(this, tr("Save Wave Data"),
                                                    QDir::currentPath() + "/wave_data.csv",
                                                    tr("CSV Files (*.csv)"));
    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "失败", "无法打开文件写入");
        return;
    }

    QTextStream out(&file);

    QMap<int, QList<QPointF>> channelData;
    int maxSamples = 0;
    for (int ch = 0; ch < 11; ++ch) {
        if (channelsEnabled[ch]) {
            channelData[ch] = wave->getWaveDataForChannel((WAVE_CH)ch);
            maxSamples = qMax(maxSamples, channelData[ch].size());
        }
    }

    out << "Time";
    for (int ch = 0; ch < 11; ++ch) {
        if (channelsEnabled[ch]) {
            out << ",Channel" << ch;
        }
    }
    out << "\n";

    for (int i = 0; i < maxSamples; ++i) {
        bool firstCol = true;
        for (int ch = 0; ch < 11; ++ch) {
            if (channelsEnabled[ch]) {
                if (firstCol) {
                    if (i < channelData[ch].size()) {
                        out << QString::number(channelData[ch][i].x(), 'f', 6);
                    }
                    firstCol = false;
                }
                out << ",";
                if (i < channelData[ch].size()) {
                    out << QString::number(channelData[ch][i].y(), 'f', 6);
                }
            }
        }
        out << "\n";
    }

    file.close();
    QMessageBox::information(this, "成功", "数据已保存到CSV文件");
}

void MainWindow::onModeComboBoxChanged(int index)
{
    currentMode = index;

    if (currentMode == 0) {
        ui->groupBox->setVisible(false);
        ui->groupBox_5->setTitle("选择通道");
    } else {
        ui->groupBox->setVisible(true);
        ui->groupBox_5->setTitle("输出通道 (固定使用前2个)");
    }
}

void MainWindow::on_pushButton_pause_clicked()
{
    aiManager->stopSampling();
    aoManager->stopOutput();
    dioManager->stopReadingInput();
    wave->pauseGraph();
}

void MainWindow::onAOWaveformPoint(double timeSec, double voltage)
{
    wave->addSeriesData(WAVE_CH11, QPointF(timeSec, voltage));
    wave->flushRange();
    gauge->setVoltage(voltage);
}

void MainWindow::onTabChanged(int index)
{
    if (index == 1) {
        gauge->show();
    } else {
        gauge->hide();
    }

    if (index == 0) {
        wave->setYAxisAutoScale(true);
        wave->setLegendVisible(true);
        for (int i = WAVE_CH0; i <= WAVE_CH10; ++i) wave->openChannel((WAVE_CH)i);
        for (int i = WAVE_CH12; i <= WAVE_CH19; ++i) wave->closeChannel((WAVE_CH)i);
        wave->openChannel(WAVE_CH11);
    } else if (index == 1) {
        wave->setYAxisAutoScale(true);
        wave->setLegendVisible(false);
        for (int i = WAVE_CH0; i <= WAVE_CH10; ++i) wave->closeChannel((WAVE_CH)i);
        for (int i = WAVE_CH12; i <= WAVE_CH19; ++i) wave->closeChannel((WAVE_CH)i);
        wave->openChannel(WAVE_CH11);
    } else if (index == 2) {
        wave->setYAxisRange(0, 13);  // 数字通道固定范围（带偏移叠加显示）
        wave->setLegendVisible(true);
        for (int i = WAVE_CH0; i <= WAVE_CH10; ++i) wave->closeChannel((WAVE_CH)i);
        for (int i = WAVE_CH12; i <= WAVE_CH19; ++i) wave->openChannel((WAVE_CH)i);
        wave->openChannel(WAVE_CH11);
    }

    wave->startGraph();
    wave->update();
}

void MainWindow::on_pushButton_dioStart_clicked()
{
    QString port = ui->comboBox_dioPort->currentText();
    dioManager->setPort(port);

    if (ui->radioButton_DO->isChecked()) {
        QVector<bool> data(8);
        for (int i = 0; i < 8; ++i)
            data[i] = m_dioCheckBoxes[i] && m_dioCheckBoxes[i]->isChecked();
        if (dioManager->writeDigitalOutput(data)) {
            QMessageBox::information(this, "成功", "数字输出已成功写入！");
        }
    } else {
        int interval = ui->spinBox_interval->value();
        dioManager->startReadingInput(interval);
    }
}

void MainWindow::on_pushButton_dioStop_clicked()
{
    dioManager->stopReadingInput();
}

void MainWindow::onDIDataReady(const QVector<bool> &data, const QVector<QPointF> &waveData)
{
    for (int i = 0; i < 8; ++i) {
        if (m_diLabels[i])
            m_diLabels[i]->setStyleSheet(data[i] ? "background-color: green" : "background-color: red");
        wave->addSeriesData((WAVE_CH)(WAVE_CH12 + i), waveData[i]);
    }
    wave->flushRange();
}

void MainWindow::on_radioButton_DO_toggled(bool checked)
{
    for (int i = 0; i < 8; ++i) {
        if (m_dioCheckBoxes[i]) m_dioCheckBoxes[i]->setEnabled(checked);
        if (m_diLabels[i])      m_diLabels[i]->setVisible(!checked);
    }
}

void MainWindow::on_pushButton_canInit_clicked()
{
    if (canManager->isOpen()) {
        canManager->closeCAN();
        ui->pushButton_canInit->setText("Init");
        return;
    }

    const QString iface = ui->comboBox_canPort->currentText();
    const uint32_t baud = ui->comboBox_canBaud->currentText().toUInt();

    if (canManager->openCAN(iface, baud)) {
        ui->pushButton_canInit->setText("Close");
        QMessageBox::information(this, "CAN", "初始化成功");
    }
}

void MainWindow::on_pushButton_canSend_clicked()
{
    if (!canManager->isOpen()) {
        QMessageBox::warning(this, "CAN", "请先初始化！");
        return;
    }

    const QByteArray payload = hexStringToBytes(ui->lineEdit_canSend->text());
    if (payload.isEmpty()) {
        QMessageBox::warning(this, "格式错误", "请输入十六进制数据");
        return;
    }

    canManager->sendFrame(0x123, payload);
}

void MainWindow::onCANFrameReceived(uint32_t id, const QByteArray &data, const QString &timestamp)
{
    QString line = QString("[%1] ID:0x%2 DLC:%3 Data:")
                       .arg(timestamp)
                       .arg(id, 3, 16, QChar('0'))
                       .arg(data.size());

    for (int i = 0; i < data.size(); ++i) {
        line += QString(" %1").arg(static_cast<unsigned char>(data[i]), 2, 16, QChar('0'));
    }

    ui->plainTextEdit_canRecv->appendPlainText(line.toUpper());
}

void MainWindow::onManagerError(const QString &error)
{
    QMessageBox::warning(this, "错误", error);
}

QByteArray MainWindow::hexStringToBytes(const QString &str)
{
    QByteArray out;
    const QRegularExpression re(R"(([0-9A-Fa-f]{2}))");
    QRegularExpressionMatchIterator it = re.globalMatch(str);
    while (it.hasNext()) {
        out.append(static_cast<char>(it.next().captured(1).toUInt(nullptr, 16)));
    }
    return out;
}

bool MainWindow::selectMasterSlavePort(QString &master, QString &slave)
{
    const auto ports = QSerialPortInfo::availablePorts();
    if (ports.size() < 2) {
        QMessageBox::warning(this, "错误", "可用串口不足两个，无法设置主从端口");
        return false;
    }

    QStringList portNames;
    for (const QSerialPortInfo &info : ports)
        portNames << info.portName();

    bool ok = false;
    master = QInputDialog::getItem(this, "选择主端口", "主端口：", portNames, 0, false, &ok);
    if (!ok || master.isEmpty()) return false;

    QStringList slaveCandidates = portNames;
    slaveCandidates.removeAll(master);

    slave = QInputDialog::getItem(this, "选择从端口", "从端口：", slaveCandidates, 0, false, &ok);
    return ok && !slave.isEmpty();
}

void MainWindow::on_pushButton_test_clicked()
{
    QString master, slave;
    if (!selectMasterSlavePort(master, slave)) return;
    QMessageBox::information(this, "设置成功", "主端口：" + master + "\n从端口：" + slave);
}

void MainWindow::on_pushButton_linOpen_clicked()
{
    QString master, slave;
    if (!selectMasterSlavePort(master, slave)) return;

    int baudRate = ui->comboBox_linBaud->currentText().toInt();
    int dataBits = ui->comboBox_linDataBits->currentText().toInt();
    int stopBits = ui->comboBox_linStopBits->currentText().toInt();

    if (linManager->openPorts(master, slave, baudRate, dataBits, stopBits)) {
        QMessageBox::information(this, "成功", "主从串口已打开");
    }
}

void MainWindow::on_pushButton_linSend_clicked()
{
    if (!linManager->isOpen()) {
        QMessageBox::warning(this, "错误", "请先打开串口");
        return;
    }

    QString input = ui->lineEdit_linSend->text().remove(" ");
    QRegularExpression re("^[0-9a-fA-F]+$");
    if (input.isEmpty() || input.length() % 2 != 0 || !re.match(input).hasMatch()) {
        QMessageBox::warning(this, "格式错误", "请输入合法的十六进制字符串，例如 55 12 或 A1B2C3");
        return;
    }

    QByteArray data = QByteArray::fromHex(input.toUtf8());
    linManager->sendData(data);
}

void MainWindow::onLINMasterDataReceived(const QByteArray &data)
{
    ui->plainTextEdit_linRecv->appendPlainText(data.toHex(' ').toUpper());
}
