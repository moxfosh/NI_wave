#ifndef CANMANAGER_H
#define CANMANAGER_H

#include <QObject>
#include <QTimer>
#include <QByteArray>
#include <QString>
#include <QDateTime>

#include <nixnet.h>

#ifndef NX_MODE_FRAME_IN_STREAM
#define NX_MODE_FRAME_IN_STREAM   nxMode_FrameInStream
#endif
#ifndef NX_MODE_FRAME_OUT_STREAM
#define NX_MODE_FRAME_OUT_STREAM  nxMode_FrameOutStream
#endif
#ifndef NX_PROP_SESSION_INTF_BAUD_RATE
#define NX_PROP_SESSION_INTF_BAUD_RATE  nxPropSession_IntfBaudRate
#endif
#ifndef NX_START_STOP_SCOPE_NORMAL
#define NX_START_STOP_SCOPE_NORMAL  nxStartStop_Normal
#endif
#ifndef NX_FRAME_TYPE_CAN_DATA
#define NX_FRAME_TYPE_CAN_DATA  nxFrameType_CAN_Data
#endif

class CANManager : public QObject
{
    Q_OBJECT

public:
    explicit CANManager(QObject *parent = nullptr);
    ~CANManager();
    bool openCAN(const QString &tinterface, uint32_t baudRate);
    void closeCAN();
    bool sendFrame(uint32_t id, const QByteArray &data);
    bool isOpen() const { return txSession != 0; }

signals:
    void frameReceived(uint32_t id, const QByteArray &data, const QString &timestamp);
    void errorOccurred(const QString &error);

private slots:
    void readFrames();

private:
    void handleNXError(nxStatus_t status);

    nxSessionRef_t txSession;
    nxSessionRef_t rxSession;
    QTimer canTimer;
};

#endif // CANMANAGER_H

