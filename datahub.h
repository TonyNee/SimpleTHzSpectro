#pragma once
#include <QObject>
#include <QVector>
#include <QByteArray>
#include <utility>
#include <QMutex>
#include <QMutexLocker>

/**
 * @brief 全局数据中心（单例）—— 替代原项目 ThreadManager
 *
 * 作为所有子系统之间的共享数据中枢。
 * UdpReceiver 写入原始包，Analysis 读取并写入处理结果，MainWindow 读取并显示。
 * 使用 QMutex 保护所有写操作，读操作在信号槽上下文中保证线程安全。
 */
class SimpleDataHub : public QObject
{
    Q_OBJECT
public:
    static SimpleDataHub& instance() {
        static SimpleDataHub hub;
        return hub;
    }

    // ---- UDP原始数据 ----
    QVector<QByteArray> pcapData;
    QMutex pcapMutex;

    // ---- DBI输出（帧分割前完整数据） ----
    QVector<float> dbiOutput;
    QMutex dbiMutex;

    // ---- 分段帧数据 ----
    QVector<QVector<float>> OSCData;
    QMutex oscMutex;

    // ---- 当前帧波形 (time, amplitude) ----
    QVector<std::pair<float, float>> waveData;
    QMutex waveMutex;

    // ---- 参数 ----
    float freqInterval = 0.307012f;
    int frameId = 0;
    int sampleTimeNs = 0;

signals:
    // 数据就绪信号（各工作线程通过单例发出，UI线程接收）
    void pcapDataReady();       // pcapData 累积完成（EOF检测到）
    void waveDataReady();       // 波形数据已更新
    void statusUpdate(const QString& msg);
    void packetCountChanged(int count);

private:
    SimpleDataHub() = default;
    ~SimpleDataHub() = default;
    SimpleDataHub(const SimpleDataHub&) = delete;
    SimpleDataHub& operator=(const SimpleDataHub&) = delete;
};
