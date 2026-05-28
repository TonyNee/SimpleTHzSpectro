#pragma once
#include <QObject>
#include <QVector>
#include <QByteArray>
#include <utility>

/**
 * @brief 单通道ADC数据分析引擎（精简自原Analysis）
 *
 * 处理流程：
 * 1. handleChannelData() - 解析UDP原始包为4通道ADC数据
 * 2. funcPcap() - DBI数字后端宽频重建
 * 3. funcADC() - 分段、归一化、构建显示用(time, amplitude)对
 * 4. localADC() - 从已有帧数据刷新显示（帧切换回放）
 *
 * 移除了原版的：OSC分析、触发信号分析、信号对齐、双通道合并
 */
class Analysis : public QObject
{
    Q_OBJECT
public:
    explicit Analysis(QObject *parent = nullptr);
    ~Analysis();

    // 独立设置两类系数文件目录
    void setCalibrationDir(const QString& dir);  // 每次开机校准：w_miso, sync_head
    void setFilterDir(const QString& dir);        // 固定滤波器：ft_before/after_mixer

    // 获取数据（供UI使用）
    QVector<std::pair<float, float>>& getWaveData();
    QVector<QVector<float>>& getOSCData();
    const QVector<float>& getDbiOutput() const;  // DBI处理后、帧分割前的完整数据
    int getFrameCount() const;
    float getFreqInterval() const;

    // 帧导航
    void setFrameId(int id);
    int getFrameId() const;

signals:
    void waveDataReady();           // 波形数据就绪，通知UI刷新
    void statusUpdate(const QString& msg);

public slots:
    void processPcapData(const QVector<QByteArray>& pcapData);  // 主入口：处理UDP累积数据
    void refreshCurrentFrame();     // 刷新当前帧显示（帧切换时调用）
    void loadDbiData(const QString& filePath);  // 从CSV加载DBI原始数据(1D)，经funcADC分割
    void loadOscData(const QString& filePath);  // 从CSV加载已分割帧数据(2D)，直接填充OSCData
    void clearData();               // 清除所有数据

private:
    void handleChannelData(const QVector<QByteArray>& pcapData);
    void funcPcap();
    void funcADC(const QVector<float>& adcData);

    // 校准文件路径（每次开机变化）
    QString calibDir;
    QString path_w_miso_0, path_w_miso_1, path_w_miso_2;
    QString path_sync_header;

    // 固定滤波器文件路径（不变的DBI设计参数）
    QString filterDir;
    QString path_ft_before_mixer, path_ft_after_mixer_0, path_ft_after_mixer_1;

    // 原始ADC通道数据
    QByteArray ch1Data, ch2Data, ch3Data, ch4Data;

    // 分析输出
    QVector<std::pair<float, float>> waveData;  // 当前帧波形 (time, amplitude)
    QVector<QVector<float>> OSCData;            // 所有分段帧
    QVector<float> m_dbiOutput;                 // DBI处理后、帧分割前的完整数据
    float freqInterval;
    int frameId;
    int sampleTime;
};
