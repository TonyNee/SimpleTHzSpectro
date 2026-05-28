#pragma once
#include <QObject>
#include <QUdpSocket>
#include <QVector>
#include <QByteArray>

/**
 * @brief UDP数据接收器 —— 替代原项目的PcapRecv
 *
 * 使用QUdpSocket直接接收ADC发送的UDP数据包（1005字节/包），
 * 累积到缓冲区中，检测到"eofeofeof"标记时触发数据分析。
 */
class UdpReceiver : public QObject
{
    Q_OBJECT
public:
    explicit UdpReceiver(QObject *parent = nullptr);
    ~UdpReceiver();

    bool bind(const QString& address, quint16 port);
    void unbind();
    bool isBound() const;

    // 发送ADC采样触发命令
    void setAdcTarget(const QString& ip, quint16 port);
    void sendADCMessage(int sampleTimeNs);

    QVector<QByteArray>& getPcapData();
    void clearBuffer();

signals:
    void dataReady();       // EOF标记检测到，触发分析
    void statusUpdate(const QString& msg);  // 状态消息
    void packetReceived(int count);         // 已接收包数量

public slots:
    void readPendingDatagrams();

private:
    QUdpSocket* mUdpSocket;
    QVector<QByteArray> mPcapData;
    bool mBound;

    QHostAddress mAdcAddr;
    quint16 mAdcPort;
};
