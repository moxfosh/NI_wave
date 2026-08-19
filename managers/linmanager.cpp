#include "linmanager.h"

LINManager::LINManager(QObject *parent)
    : QObject(parent)
{
    connect(&masterSerial, &QSerialPort::readyRead, this, &LINManager::onMasterDataReady);
    connect(&slaveSerial, &QSerialPort::readyRead, this, &LINManager::onSlaveDataReady);
}

LINManager::~LINManager()
{
    closePorts();
}

bool LINManager::openPorts(const QString &masterPort, const QString &slavePort,
                          int baudRate, int dataBits, int stopBits)
{
    if (masterSerial.isOpen()) masterSerial.close();
    if (slaveSerial.isOpen()) slaveSerial.close();

    masterSerial.setPortName(masterPort);
    masterSerial.setBaudRate(baudRate);
    masterSerial.setDataBits(static_cast<QSerialPort::DataBits>(dataBits));
    masterSerial.setStopBits(static_cast<QSerialPort::StopBits>(stopBits));
    masterSerial.setParity(QSerialPort::NoParity);
    masterSerial.setFlowControl(QSerialPort::NoFlowControl);

    if (!masterSerial.open(QIODevice::ReadWrite)) {
        emit errorOccurred(QString("Failed to open master port: %1").arg(masterPort));
        return false;
    }

    slaveSerial.setPortName(slavePort);
    slaveSerial.setBaudRate(baudRate);
    slaveSerial.setDataBits(static_cast<QSerialPort::DataBits>(dataBits));
    slaveSerial.setStopBits(static_cast<QSerialPort::StopBits>(stopBits));
    slaveSerial.setParity(QSerialPort::NoParity);
    slaveSerial.setFlowControl(QSerialPort::NoFlowControl);

    if (!slaveSerial.open(QIODevice::ReadWrite)) {
        emit errorOccurred(QString("Failed to open slave port: %1").arg(slavePort));
        masterSerial.close();
        return false;
    }

    masterPortName = masterPort;
    slavePortName = slavePort;

    return true;
}

void LINManager::closePorts()
{
    if (masterSerial.isOpen()) {
        masterSerial.close();
    }
    if (slaveSerial.isOpen()) {
        slaveSerial.close();
    }
}

bool LINManager::sendData(const QByteArray &data)
{
    if (!masterSerial.isOpen()) {
        emit errorOccurred("Master port not open");
        return false;
    }

    qint64 written = masterSerial.write(data);
    return written == data.size();
}

void LINManager::onMasterDataReady()
{
    QByteArray data = masterSerial.readAll();
    emit masterDataReceived(data);
}

void LINManager::onSlaveDataReady()
{
    QByteArray data = slaveSerial.readAll();
    emit slaveDataReceived(data);
}
