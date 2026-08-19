#ifndef LINMANAGER_H
#define LINMANAGER_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QByteArray>

class LINManager : public QObject
{
    Q_OBJECT

public:
    explicit LINManager(QObject *parent = nullptr);
    ~LINManager();

    bool openPorts(const QString &masterPort, const QString &slavePort,
                   int baudRate, int dataBits, int stopBits);
    void closePorts();
    bool sendData(const QByteArray &data);
    bool isOpen() const { return masterSerial.isOpen() && slaveSerial.isOpen(); }

    QString getMasterPort() const { return masterPortName; }
    QString getSlavePort() const { return slavePortName; }

signals:
    void masterDataReceived(const QByteArray &data);
    void slaveDataReceived(const QByteArray &data);
    void errorOccurred(const QString &error);

private slots:
    void onMasterDataReady();
    void onSlaveDataReady();

private:
    QSerialPort masterSerial;
    QSerialPort slaveSerial;
    QString masterPortName;
    QString slavePortName;
};

#endif // LINMANAGER_H
