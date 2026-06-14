#pragma once
#include <QObject>
#include <QThread>
#include <QUdpSocket>
#include <QHostAddress>

/**
 * @brief UDP数据接收器（独立线程）—— 替代原项目 PcapRecv + UDP 控制通道
 *
 * 运行在独立 QThread 中，负责：
 * 1. 绑定指定IP:端口监听ADC数据包
 * 2. 向ADC发送采样触发命令（"mv424"协议）
 * 3. 接收所有UDP包写入 SimpleDataHub::pcapData
 * 4. 检测EOF包（23字节含"eofeofeof"）→ 触发 dataReady
 *
 * 与原项目对应关系：
 *   原 UDP::sendADCMessage  →  UdpReceiver::sendADCMessage
 *   原 PcapRecv::packetHandler → UdpReceiver::readPendingDatagrams
 *   原 ThreadManager::pcapData → SimpleDataHub::pcapData
 */
class UdpReceiver : public QObject
{
    Q_OBJECT
public:
    explicit UdpReceiver(QObject *parent = nullptr);
    ~UdpReceiver();

    // 线程控制
    void startWorker(const QString& bindIP, quint16 bindPort,
                     const QString& adcIP, quint16 adcPort);
    void stopWorker();

    bool isBound() const;

    // 发送ADC采样触发命令（可在任意线程调用，信号槽跨线程）
    void sendADCMessage(int sampleTimeNs);

signals:
    void statusUpdate(const QString& msg);
    void packetCountChanged(int count);

    // 内部信号：在线程内触发发送
    void doSendADCMessage(int sampleTimeNs);

private slots:
    void readPendingDatagrams();
    void onSendADCMessage(int sampleTimeNs);

private:
    void initSocket();

    QThread* m_workerThread;
    QUdpSocket* mUdpSocket;
    bool mBound;

    QString mBindIP;
    quint16 mBindPort;
    QHostAddress mAdcAddr;
    quint16 mAdcPort;
};
