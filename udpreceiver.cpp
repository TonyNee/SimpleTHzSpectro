#include "udpreceiver.h"
#include "datahub.h"
#include <QDebug>
#include <cmath>

// DBI参数：截断长度 (sync_delay余量 + equalizer长度)
#define LEN_CUT (500 + 1000)

UdpReceiver::UdpReceiver(QObject *parent)
    : QObject(parent), m_workerThread(nullptr),
      mUdpSocket(nullptr), mBound(false),
      mAdcAddr(QHostAddress("10.10.229.11")), mAdcPort(5506)
{
}

UdpReceiver::~UdpReceiver()
{
    stopWorker();
}

void UdpReceiver::startWorker(const QString& bindIP, quint16 bindPort,
                               const QString& adcIP, quint16 adcPort)
{
    stopWorker();  // 先停旧线程

    mBindIP = bindIP;
    mBindPort = bindPort;
    mAdcAddr.setAddress(adcIP);
    mAdcPort = adcPort;

    m_workerThread = new QThread();
    this->moveToThread(m_workerThread);

    // 线程启动时初始化socket
    connect(m_workerThread, &QThread::started, this, &UdpReceiver::initSocket);
    // 内部信号：跨线程发ADC命令
    connect(this, &UdpReceiver::doSendADCMessage,
            this, &UdpReceiver::onSendADCMessage);

    m_workerThread->start();
}

void UdpReceiver::stopWorker()
{
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait();
        delete m_workerThread;
        m_workerThread = nullptr;
    }
    if (mUdpSocket) {
        mUdpSocket->close();
        delete mUdpSocket;
        mUdpSocket = nullptr;
    }
    mBound = false;
}

bool UdpReceiver::isBound() const
{
    return mBound;
}

void UdpReceiver::initSocket()
{
    mUdpSocket = new QUdpSocket();

    // 设置大接收缓冲区 (64MB) —— 高速UDP数据流关键
    mUdpSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,
                                 64 * 1024 * 1024);

    QHostAddress hostAddr(mBindIP);
    if (!mUdpSocket->bind(hostAddr, mBindPort)) {
        qWarning() << "UdpReceiver: Failed to bind to" << mBindIP << ":" << mBindPort;
        emit statusUpdate(QString("Bind failed: %1:%2").arg(mBindIP).arg(mBindPort));
        delete mUdpSocket;
        mUdpSocket = nullptr;
        mBound = false;
        return;
    }

    // readyRead 在线程事件循环中触发 → 直接执行不阻塞
    connect(mUdpSocket, &QUdpSocket::readyRead,
            this, &UdpReceiver::readPendingDatagrams);

    mBound = true;
    emit statusUpdate(QString("Listening on %1:%2  (buf=64MB)").arg(mBindIP).arg(mBindPort));
    qDebug() << "UdpReceiver: Bound to" << mBindIP << ":" << mBindPort << "buffer=64MB";
}

void UdpReceiver::readPendingDatagrams()
{
    if (!mUdpSocket) return;

    SimpleDataHub& hub = SimpleDataHub::instance();
    QHostAddress addr;
    quint16 port;
    QByteArray arr;
    QString dataString;

    // 一次性读完所有待处理数据报，拼接后检测EOF标记（参考源程序逻辑）
    while (mUdpSocket->hasPendingDatagrams())
    {
        arr.resize(mUdpSocket->pendingDatagramSize());
        mUdpSocket->readDatagram(arr.data(), arr.size(), &addr, &port);

        // 所有包（无论大小）都追加到缓冲区
        hub.pcapData.append(arr);
        // 同时拼接字符串用于EOF内容检测
        dataString += QString::fromUtf8(arr.data());
    }

    int totalPkts = hub.pcapData.size();
    if (totalPkts > 0) {
        emit packetCountChanged(totalPkts);
    }

    // 通过内容检测EOF标记（参考源程序: 检查 "EOF\r\n" 或 "eofeofeof"）
    if (dataString.endsWith("EOF\r\n") || dataString.contains("eofeofeof")) {
        qDebug() << "UdpReceiver: EOF detected, total pkts:" << totalPkts;
        emit hub.pcapDataReady();
    }
}

void UdpReceiver::sendADCMessage(int sampleTimeNs)
{
    // 跨线程安全：通过信号槽将发送请求投递到worker线程
    emit doSendADCMessage(sampleTimeNs);
}

void UdpReceiver::onSendADCMessage(int sampleTimeNs)
{
    if (!mUdpSocket) {
        emit statusUpdate("Error: UDP socket not bound");
        return;
    }

    // 计算所需UDP包数: ceil((sampleTime_ns * 120 GSa/s + len_cut) / 192 / 16 / 3)
    int pktNum = static_cast<int>(ceil((sampleTimeNs * 120.0 + LEN_CUT) / 192.0 / 16 / 3));

    // 清空上一轮数据
    SimpleDataHub& hub = SimpleDataHub::instance();
    hub.pcapData.clear();
    hub.sampleTimeNs = sampleTimeNs;

    // 构建协议包: "mv424" (hex: 6D76343234) + 3字节包数 (大端)
    QByteArray arr = QByteArray::fromHex("6D76343234");
    arr.append(static_cast<char>((pktNum >> 16) & 0xFF));
    arr.append(static_cast<char>((pktNum >> 8) & 0xFF));
    arr.append(static_cast<char>(pktNum & 0xFF));

    qint64 sent = mUdpSocket->writeDatagram(arr, mAdcAddr, mAdcPort);
    emit statusUpdate(QString("Sent ADC trigger: %1ns → %2 pkts to %3:%4")
        .arg(sampleTimeNs).arg(pktNum)
        .arg(mAdcAddr.toString()).arg(mAdcPort));
    qDebug() << "UdpReceiver: Sent" << sent << "bytes to" << mAdcAddr.toString() << ":" << mAdcPort;
}
