#include "canmanager.h"

CANManager::CANManager(QObject *parent)
    : QObject(parent)
    , txSession(0)
    , rxSession(0)
{
    connect(&canTimer, &QTimer::timeout, this, &CANManager::readFrames);
}

CANManager::~CANManager()
{
    closeCAN();
}

bool CANManager::openCAN(const QString &tinterface, uint32_t baudRate)
{
    nxStatus_t st;

    st = nxCreateSession(":memory:", "", "", tinterface.toUtf8().constData(),
                         NX_MODE_FRAME_IN_STREAM, &rxSession);
    if (st) {
        handleNXError(st);
        return false;
    }

    st = nxCreateSession(":memory:", "", "", tinterface.toUtf8().constData(),
                         NX_MODE_FRAME_OUT_STREAM, &txSession);
    if (st) {
        handleNXError(st);
        closeCAN();
        return false;
    }

    st = nxSetProperty(rxSession, NX_PROP_SESSION_INTF_BAUD_RATE,
                       sizeof(baudRate), &baudRate);
    if (st) {
        handleNXError(st);
        closeCAN();
        return false;
    }

    st = nxSetProperty(txSession, NX_PROP_SESSION_INTF_BAUD_RATE,
                       sizeof(baudRate), &baudRate);
    if (st) {
        handleNXError(st);
        closeCAN();
        return false;
    }

    if ((st = nxStart(rxSession, NX_START_STOP_SCOPE_NORMAL)) ||
        (st = nxStart(txSession, NX_START_STOP_SCOPE_NORMAL))) {
        handleNXError(st);
        closeCAN();
        return false;
    }

    canTimer.start(10);
    return true;
}

void CANManager::closeCAN()
{
    canTimer.stop();

    if (rxSession) {
        nxStop(rxSession, NX_START_STOP_SCOPE_NORMAL);
        nxClear(rxSession);
        rxSession = 0;
    }

    if (txSession) {
        nxStop(txSession, NX_START_STOP_SCOPE_NORMAL);
        nxClear(txSession);
        txSession = 0;
    }
}

bool CANManager::sendFrame(uint32_t id, const QByteArray &data)
{
    if (!txSession) {
        emit errorOccurred("CAN not initialized");
        return false;
    }

    if (id > 0x1FFFFFFF) {
        emit errorOccurred("Invalid CAN frame ID (max 0x1FFFFFFF)");
        return false;
    }

    if (data.isEmpty() || data.size() > 8) {
        emit errorOccurred("Invalid data size (1-8 bytes)");
        return false;
    }

    nxFrameVar_t frame{};
    frame.Identifier = id;
    frame.Type = NX_FRAME_TYPE_CAN_DATA;
    frame.PayloadLength = static_cast<u8>(data.size());
    memcpy(frame.Payload, data.constData(), data.size());

    nxStatus_t st = nxWriteFrame(txSession, &frame, sizeof(frame), 1000);
    if (st) {
        handleNXError(st);
        return false;
    }

    return true;
}

void CANManager::readFrames()
{
    if (!rxSession) return;

    nxFrameVar_t frame{};
    u32 nFrames = 0;

    nxStatus_t st = nxReadFrame(rxSession, &frame, sizeof(frame), 0, &nFrames);
    if (st || nFrames == 0) return;

    QByteArray data(reinterpret_cast<const char*>(frame.Payload), frame.PayloadLength);
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");

    emit frameReceived(frame.Identifier, data, timestamp);
}

void CANManager::handleNXError(nxStatus_t status)
{
    char msg[256];
    nxStatusToString(status, sizeof(msg), msg);
    emit errorOccurred(QString("CAN Error: %1").arg(msg));
}
