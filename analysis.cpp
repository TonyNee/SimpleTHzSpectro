#include "analysis.h"
#include "datahub.h"
#include "DBI/dbi.h"
#include "DBI/operate_file.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <cmath>
#include <limits>

// 前置声明
static void parseChannelData(const QVector<QByteArray>& pcapData,
                             QByteArray& ch1, QByteArray& ch2,
                             QByteArray& ch3, QByteArray& ch4);

Analysis::Analysis(QObject *parent)
    : QObject(parent)
{
    calibDir = "Config/Calibration";
    filterDir = "Config/Filters";

    path_w_miso_0 = calibDir + "/w_miso_dev1_1.txt";
    path_w_miso_1 = calibDir + "/w_miso_dev1_2.txt";
    path_w_miso_2 = calibDir + "/w_miso_dev1_3.txt";
    path_sync_header = calibDir + "/sync_head_dev1.txt";

    path_ft_before_mixer  = filterDir + "/ft_before_mixer.txt";
    path_ft_after_mixer_0 = filterDir + "/ft_after_mixer_1.txt";
    path_ft_after_mixer_1 = filterDir + "/ft_after_mixer_2.txt";
}

Analysis::~Analysis() {}

void Analysis::setCalibrationDir(const QString& dir)
{
    calibDir = dir;
    path_w_miso_0 = calibDir + "/w_miso_dev1_1.txt";
    path_w_miso_1 = calibDir + "/w_miso_dev1_2.txt";
    path_w_miso_2 = calibDir + "/w_miso_dev1_3.txt";
    path_sync_header = calibDir + "/sync_head_dev1.txt";
}

void Analysis::setFilterDir(const QString& dir)
{
    filterDir = dir;
    path_ft_before_mixer  = filterDir + "/ft_before_mixer.txt";
    path_ft_after_mixer_0 = filterDir + "/ft_after_mixer_1.txt";
    path_ft_after_mixer_1 = filterDir + "/ft_after_mixer_2.txt";
}

// ====================== 主入口：从 SimpleDataHub 读数据 ======================

void Analysis::onPcapDataReady()
{
    SimpleDataHub& hub = SimpleDataHub::instance();

    // 从单例复制原始数据
    QVector<QByteArray> localPcap = hub.pcapData;
    hub.pcapData.clear();

    if (localPcap.isEmpty()) {
        emit statusUpdate("Analysis: Empty pcap data");
        return;
    }

    emit statusUpdate(QString("Analysis: Processing %1 packets...").arg(localPcap.size()));

    // 解析通道数据
    parseChannelData(localPcap, ch1Data, ch2Data, ch3Data, ch4Data);

    int dataPktCount = 0;
    for (const auto& p : localPcap) { if (p.size() == 1005) dataPktCount++; }

    if (ch1Data.isEmpty() || ch2Data.isEmpty() || ch3Data.isEmpty() || ch4Data.isEmpty()) {
        qWarning() << "Analysis: ADC channel data is empty!";
        emit statusUpdate("Analysis: Error - No ADC data packets found");
        return;
    }

    emit statusUpdate(QString("Analysis: %1 data pkts → ch1=%2B ch2=%3B ch3=%4B ch4=%5B")
        .arg(dataPktCount)
        .arg(ch1Data.size()).arg(ch2Data.size())
        .arg(ch3Data.size()).arg(ch4Data.size()));

    funcPcap();
}

// 实际工作函数：传入本地pcapData副本进行处理
static void parseChannelData(const QVector<QByteArray>& pcapData,
                             QByteArray& ch1, QByteArray& ch2,
                             QByteArray& ch3, QByteArray& ch4)
{
    ch1.clear(); ch2.clear(); ch3.clear(); ch4.clear();

    for (const QByteArray& packetData : pcapData) {
        // 只处理1005字节的ADC数据包，跳过EOF包（23字节）
        if (packetData.size() != 1005) {
            continue;
        }

        QByteArray channel1Data = packetData.mid(0, 192);
        QByteArray channel2Data = packetData.mid(200, 192);
        QByteArray channel3Data = packetData.mid(400, 192);
        QByteArray channel4Data = packetData.mid(600, 192);

        ch1.append(channel1Data);
        ch2.append(channel2Data);
        ch3.append(channel3Data);
        ch4.append(channel4Data);
    }
}

// ====================== DBI处理管线 ======================

void Analysis::funcPcap()
{
    SimpleDataHub& hub = SimpleDataHub::instance();
    double lo_freq[ch_num - 1] = { 34.4, 32 };

    // *******************************   Way1: ADC sample data  *********************************** //

    // 获取每个通道的实际数据大小（字节数）
    int len_ch1 = ch1Data.size();
    int len_ch2 = ch2Data.size();
    int len_ch3 = ch4Data.size();

    // len_in 基于实际收到的数据，避免理论与实际不一致导致DBI越界崩溃
    int len_in = len_ch1;

    // len_out = sample_time(ns) * 120 GSa/s，但需确保不超出DBI内部缓冲区
    // DBI步骤8需要: len_out + sync_delay_max + 2000 <= len_dbi = len_in * ch_num
    int len_out_theoretical = hub.sampleTimeNs * 120;
    int len_out = len_out_theoretical;
    if (len_out + 3000 > len_in * ch_num) {  // 3000 = sync_delay余量 + 2000
        len_out = len_in * ch_num - 3000;    // 收缩到安全范围
    }

    // 分配内存并转换int8 → double
    double* adc_data[ch_num];
    adc_data[0] = (double*)malloc(sizeof(double) * len_ch1);
    adc_data[1] = (double*)malloc(sizeof(double) * len_ch2);
    adc_data[2] = (double*)malloc(sizeof(double) * len_ch3);

    if (!adc_data[0] || !adc_data[1] || !adc_data[2]) {
        qCritical() << "Analysis: Memory allocation failed!";
        emit statusUpdate("Analysis: Error - Memory allocation failed");
        return;
    }

    for (int i = 0; i < len_ch1; ++i)
        adc_data[0][i] = static_cast<double>(static_cast<qint8>(ch1Data[i]));
    for (int i = 0; i < len_ch2; ++i)
        adc_data[1][i] = static_cast<double>(static_cast<qint8>(ch2Data[i]));
    for (int i = 0; i < len_ch3; ++i)
        adc_data[2][i] = static_cast<double>(static_cast<qint8>(ch4Data[i]));

    // 加载DBI滤波器系数
    int* sync_delay = load_file_sync(path_sync_header.toLocal8Bit().constData(), 3);
    double* ft_before_mixer = load_file(path_ft_before_mixer.toLocal8Bit().constData(), len_filter);
    double* ft_after_mixer[ch_num - 1];
    ft_after_mixer[0] = load_file(path_ft_after_mixer_0.toLocal8Bit().constData(), len_filter);
    ft_after_mixer[1] = load_file(path_ft_after_mixer_1.toLocal8Bit().constData(), len_filter);
    double* w_miso[ch_num];
    w_miso[0] = load_file(path_w_miso_0.toLocal8Bit().constData(), len_equalizer);
    w_miso[1] = load_file(path_w_miso_1.toLocal8Bit().constData(), len_equalizer);
    w_miso[2] = load_file(path_w_miso_2.toLocal8Bit().constData(), len_equalizer);

    emit statusUpdate("Analysis: Running DBI pipeline...");

    // 执行DBI处理
    double* output_data = DBI_process(
        adc_data, len_out, len_in, w_miso, sync_delay,
        ft_before_mixer, ft_after_mixer, lo_freq);

    emit statusUpdate(QString("DBI output length: %1 pts").arg(len_out));

    // 转换为float并归一化
    QVector<float> dbiVec;
    for (int i = 0; i < len_out; ++i) {
        dbiVec.append(static_cast<float>(output_data[i] / 150.0));
    }

    // 写入SimpleDataHub
    hub.dbiOutput = dbiVec;

    // 后处理：分段+构建波形
    funcADC(dbiVec);

    // 自动保存ADC原始数据到 Output/ 目录（参考源程序 writeAdcDataToCSV）
    QDir outputDir("Output");
    if (!outputDir.exists()) {
        outputDir.mkpath(".");
    }
    QFile adcFile("Output/adc_data_ch1.csv");
    if (adcFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&adcFile);
        out << "ch1Data,ch2Data,ch3Data\n";
        for (int i = 0; i < len_ch1; ++i) {
            out << adc_data[0][i] << ","
                << adc_data[1][i] << ","
                << adc_data[2][i] << "\n";
        }
        adcFile.close();
        emit statusUpdate("ADC data saved to Output/adc_data_ch1.csv");
    } else {
        emit statusUpdate("Error: Cannot write Output/adc_data_ch1.csv");
    }

    // 释放内存
    for (int i = 0; i < ch_num; i++) { free(adc_data[i]); free(w_miso[i]); }
    for (int i = 0; i < ch_num - 1; i++) free(ft_after_mixer[i]);
    free(ft_before_mixer);
    free(output_data);
    free(sync_delay);

    emit statusUpdate(QString("Analysis: Complete - %1 frames, %2 pts DBI output")
        .arg(hub.OSCData.size()).arg(hub.dbiOutput.size()));
}

// ====================== ADC后处理：分段 + 构建波形 ======================

void Analysis::funcADC(const QVector<float>& adcData)
{
    SimpleDataHub& hub = SimpleDataHub::instance();

    // 每2400点找一次峰值位置（帧同步）
    std::vector<int> maxIndices;
    int count = 0;
    float maxValue = std::numeric_limits<float>::lowest();
    int maxIndex = -1;

    for (int i = 0; i < adcData.size(); ++i) {
        float value = adcData[i];
        if (value > maxValue) { maxValue = value; maxIndex = count; }
        count++;
        if (count % 2400 == 0) {
            maxIndices.push_back(maxIndex);
            maxValue = std::numeric_limits<float>::lowest();
            maxIndex = -1;
        }
    }
    if (count % 2400 != 0) maxIndices.push_back(maxIndex);

    // 频率参数
    float calibratinFreq = 370.0f;
    float samplingFrequency = 120.0f;
    float timeInterval = std::pow(10.0f, -9) / samplingFrequency;
    float fInterval = static_cast<float>(timeInterval / (2 * M_PI * 4320) * std::pow(10.0, 15));
    hub.freqInterval = fInterval;

    int dLeft = static_cast<int>((calibratinFreq - 200) / fInterval + 1);
    int dRight = static_cast<int>((910 - calibratinFreq) / fInterval + 1);
    int zeroC = static_cast<int>(200 / fInterval);

    QVector<QVector<float>> finalData;
    QVector<float> bu(zeroC, 0.0f);

    for (int m = static_cast<int>(maxIndices.size() * 0.1);
         m < static_cast<int>(maxIndices.size()); ++m) {
        int lc = maxIndices[m];
        int lLeft = lc + dLeft;
        int lRight = lc - dRight;
        if (lRight < 0 || lLeft > adcData.size() - 1) continue;

        QVector<float> data;
        data.append(bu);
        for (int n = lLeft; n >= lRight; --n) data.append(adcData[n]);
        finalData.append(data);
    }

    // 写入SimpleDataHub
    hub.OSCData.clear();
    hub.OSCData.append(finalData);

    if (finalData.isEmpty()) {
        emit statusUpdate("Analysis: Warning - No valid frames extracted");
        return;
    }

    // 构建首帧波形
    hub.waveData.clear();
    float ftime = 0;
    QVector<float> temp = finalData[0];
    for (int var = 0; var < temp.size(); ++var) {
        hub.waveData.append(std::make_pair(ftime, temp[var]));
        ftime += fInterval;
    }
    hub.frameId = 0;

    emit hub.waveDataReady();
}

// ====================== 帧切换 ======================

void Analysis::refreshCurrentFrame()
{
    SimpleDataHub& hub = SimpleDataHub::instance();

    QVector<QVector<float>> oscCopy = hub.OSCData;
    int fid = hub.frameId;
    float fInterval = hub.freqInterval;

    if (oscCopy.isEmpty() || fid < 0 || fid >= oscCopy.size()) return;

    hub.waveData.clear();
    float ftime = 0;
    QVector<float> temp = oscCopy[fid];
    for (int var = 0; var < temp.size(); ++var) {
        hub.waveData.append(std::make_pair(ftime, temp[var]));
        ftime += fInterval;
    }

    emit hub.waveDataReady();
}

// ====================== CSV加载 ======================

void Analysis::loadOscData(const QString& filePath)
{
    SimpleDataHub& hub = SimpleDataHub::instance();
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit statusUpdate("Cannot open: " + filePath);
        return;
    }

    QVector<QVector<float>> loaded;
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;
        QVector<float> frame;
        for (const QString& v : line.split(","))
            frame.append(v.toFloat());
        if (!frame.isEmpty()) loaded.append(frame);
    }
    file.close();

    if (loaded.isEmpty()) { emit statusUpdate("Empty file: " + filePath); return; }

    float samplingFrequency = 120.0f;
    float timeInterval = std::pow(10.0f, -9) / samplingFrequency;
    float fInterval = static_cast<float>(timeInterval / (2 * M_PI * 4320) * std::pow(10.0, 15));

    hub.OSCData = loaded;
    hub.freqInterval = fInterval;
    hub.frameId = 0;

    hub.waveData.clear();
    float ftime = 0;
    const QVector<float>& frame = loaded[0];
    for (int i = 0; i < frame.size(); ++i) {
        hub.waveData.append(std::make_pair(ftime, frame[i]));
        ftime += fInterval;
    }

    emit statusUpdate(QString("Loaded: %1 frames, %2 pts/frame")
        .arg(loaded.size()).arg(loaded[0].size()));
    emit hub.waveDataReady();
}

void Analysis::loadDbiData(const QString& filePath)
{
    SimpleDataHub& hub = SimpleDataHub::instance();
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit statusUpdate("Cannot open: " + filePath);
        return;
    }

    QVector<float> raw;
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (!line.isEmpty()) {
            bool ok = false;
            float val = line.toFloat(&ok);
            if (ok) raw.append(val);
        }
    }
    file.close();

    if (raw.isEmpty()) { emit statusUpdate("Empty file: " + filePath); return; }

    hub.dbiOutput = raw;

    funcADC(raw);
    emit statusUpdate(QString("Loaded DBI: %1 pts, %2 frames")
        .arg(raw.size()).arg(hub.OSCData.size()));
}

void Analysis::clearData()
{
    SimpleDataHub& hub = SimpleDataHub::instance();
    hub.pcapData.clear();
    hub.dbiOutput.clear();
    hub.OSCData.clear();
    hub.waveData.clear();
    hub.frameId = 0;
    ch1Data.clear(); ch2Data.clear(); ch3Data.clear(); ch4Data.clear();
}
