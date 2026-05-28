#include "udpreceiver.h"
#include <QDebug>
#include <cmath>

// DBI参数：截断长度
#define LEN_CUT (500 + 1000)

UdpReceiver::UdpReceiver(QObject *parent)
    : QObject(parent), mUdpSocket(nullptr), mBound(false),
      mAdcAddr(QHostAddress("10.10.229.11")), mAdcPort(5506)
{
}

UdpReceiver::~UdpReceiver()
{
    unbind();
}

bool UdpReceiver::bind(const QString& address, quint16 port)
{
    if (mUdpSocket) {
        unbind();
    }

    mUdpSocket = new QUdpSocket(this);

    QHostAddress hostAddr(address);
    if (!mUdpSocket->bind(hostAddr, port)) {
        qWarning() << "UdpReceiver: Failed to bind to" << address << ":" << port;
        delete mUdpSocket;
        mUdpSocket = nullptr;
        mBound = false;
        emit statusUpdate(QString("Bind failed: %1:%2").arg(address).arg(port));
        return false;
    }

    connect(mUdpSocket, &QUdpSocket::readyRead, this, &UdpReceiver::readPendingDatagrams);

    mBound = true;
    emit statusUpdate(QString("Listening on %1:%2").arg(address).arg(port));
    qDebug() << "UdpReceiver: Bound to" << address << ":" << port;
    return true;
}

void UdpReceiver::unbind()
{
    if (mUdpSocket) {
        mUdpSocket->close();
        delete mUdpSocket;
        mUdpSocket = nullptr;
    }
    mBound = false;
    mPcapData.clear();
}

bool UdpReceiver::isBound() const
{
    return mBound;
}

QVector<QByteArray>& UdpReceiver::getPcapData()
{
    return mPcapData;
}

void UdpReceiver::clearBuffer()
{
    mPcapData.clear();
}

void UdpReceiver::readPendingDatagrams()
{
    if (!mUdpSocket) return;

    QHostAddress addr;
    quint16 port;
    QByteArray arr;

    while (mUdpSocket->hasPendingDatagrams())
    {
        arr.resize(mUdpSocket->bytesAvailable());
        mUdpSocket->readDatagram(arr.data(), arr.size(), &addr, &port);

        // 按包长度过滤：1005字节 = ADC数据包；检测EOF标记
        if (arr.size() == 1005) {
            mPcapData.append(arr);
        } else if (arr.contains("eofeofeof")) {
            mPcapData.append(arr);
            qDebug() << "UdpReceiver: EOF detected, total pkts:" << mPcapData.size();
            emit dataReady();
        } else if (arr.size() > 0) {
            // 其他长度但包含数据的包也接受
            mPcapData.append(arr);
            // 检查是否包含EOF标记
            if (arr.contains("eofeofeof")) {
                qDebug() << "UdpReceiver: EOF detected in non-standard packet";
                emit dataReady();
            }
        }
    }

    emit packetReceived(mPcapData.size());
}

void UdpReceiver::setAdcTarget(const QString& ip, quint16 port)
{
    mAdcAddr.setAddress(ip);
    mAdcPort = port;
}

void UdpReceiver::sendADCMessage(int sampleTimeNs)
{
    if (!mUdpSocket) {
        emit statusUpdate("Error: UDP socket not bound");
        return;
    }

    // 计算所需包数: ceil((sampleTime*120 + len_cut) / 192 / 16 / 3)
    int pktNum = ceil((sampleTimeNs * 120.0 + LEN_CUT) / 192.0 / 16 / 3);

    // 构建协议包: "mv424" (hex: 6D76343234) + 3字节包数 (大端)
    QByteArray arr = QByteArray::fromHex("6D76343234");
    arr.append(static_cast<char>((pktNum >> 16) & 0xFF));
    arr.append(static_cast<char>((pktNum >> 8) & 0xFF));
    arr.append(static_cast<char>(pktNum & 0xFF));

    qint64 sent = mUdpSocket->writeDatagram(arr, mAdcAddr, mAdcPort);
    emit statusUpdate(QString("Sent ADC trigger: sampleTime=%1ns, pkts=%2, bytes=%3")
        .arg(sampleTimeNs).arg(pktNum).arg(sent));
}
