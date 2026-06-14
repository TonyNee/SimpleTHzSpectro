#pragma once
#include <QObject>
#include <QVector>
#include <QByteArray>
#include <utility>

/**
 * @brief 单通道ADC数据分析引擎
 *
 * 运行在独立 QThread 中。通过 SimpleDataHub 单例读取原始数据、
 * 写入处理结果，与原项目 Analysis + ThreadManager 的架构一致。
 *
 * 处理流程：
 * 1. onPcapDataReady() - 从 SimpleDataHub::pcapData 读原始包
 * 2. handleChannelData()  - 解析4通道ADC数据
 * 3. funcPcap()           - DBI数字后端宽频重建
 * 4. funcADC()            - 分段 + 构建波形
 */
class Analysis : public QObject
{
    Q_OBJECT
public:
    explicit Analysis(QObject *parent = nullptr);
    ~Analysis();

    void setCalibrationDir(const QString& dir);
    void setFilterDir(const QString& dir);

public slots:
    void onPcapDataReady();       // SimpleDataHub::pcapDataReady 的处理槽
    void refreshCurrentFrame();   // 帧切换
    void loadOscData(const QString& filePath);
    void loadDbiData(const QString& filePath);
    void clearData();

signals:
    void statusUpdate(const QString& msg);

private:
    void handleChannelData();
    void funcPcap();
    void funcADC(const QVector<float>& adcData);

    QString calibDir;
    QString filterDir;
    QString path_w_miso_0, path_w_miso_1, path_w_miso_2;
    QString path_sync_header;
    QString path_ft_before_mixer, path_ft_after_mixer_0, path_ft_after_mixer_1;

    QByteArray ch1Data, ch2Data, ch3Data, ch4Data;
};
