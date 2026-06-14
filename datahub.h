#pragma once
#include <QObject>
#include <QVector>
#include <QByteArray>
#include <utility>

/**
 * @brief 全局数据中心（单例）—— 替代原项目 ThreadManager
 *
 * 作为所有子系统之间的共享数据中枢。
 * UdpReceiver 写入原始包，Analysis 读取并写入处理结果，MainWindow 读取并显示。
 * 数据流严格串行：采样线程完成 → 发信号 → 分析线程处理 → 发信号 → UI线程读取，
 * 不存在并发访问，无需互斥锁。
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

    // ---- DBI输出（帧分割前完整数据） ----
    QVector<float> dbiOutput;       // 归一化后的float数据（/150），用于波形显示
    QVector<double> dbiRawOutput;   // 原始double数据（=源程序 output_data），用于文件保存

    // ---- 分段帧数据 ----
    QVector<QVector<float>> OSCData;

    // ---- 当前帧波形 (time, amplitude) ----
    QVector<std::pair<float, float>> waveData;

    // ---- 参数 ----
    float freqInterval = 0.307012f;
    int frameId = 0;
    int sampleTimeNs = 20000;  // 默认20µs

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
