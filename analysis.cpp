#include "analysis.h"
#include "DBI/dbi.h"
#include "DBI/operate_file.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <cmath>
#include <limits>

Analysis::Analysis(QObject *parent)
    : QObject(parent), freqInterval(0.307012f), frameId(0), sampleTime(0)
{
    calibDir = "Config/Calibration";
    filterDir = "Config/Filters";

    // 校准文件：每次开机需重新校准
    path_w_miso_0 = calibDir + "/w_miso_dev1_1.txt";
    path_w_miso_1 = calibDir + "/w_miso_dev1_2.txt";
    path_w_miso_2 = calibDir + "/w_miso_dev1_3.txt";
    path_sync_header = calibDir + "/sync_head_dev1.txt";

    // 固定滤波器：部署后不变
    path_ft_before_mixer  = filterDir + "/ft_before_mixer.txt";
    path_ft_after_mixer_0 = filterDir + "/ft_after_mixer_1.txt";
    path_ft_after_mixer_1 = filterDir + "/ft_after_mixer_2.txt";
}

Analysis::~Analysis()
{
}

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

QVector<std::pair<float, float>>& Analysis::getWaveData()
{
    return waveData;
}

QVector<QVector<float>>& Analysis::getOSCData()
{
    return OSCData;
}

const QVector<float>& Analysis::getDbiOutput() const
{
    return m_dbiOutput;
}

int Analysis::getFrameCount() const
{
    return OSCData.size();
}

float Analysis::getFreqInterval() const
{
    return freqInterval;
}

void Analysis::setFrameId(int id)
{
    if (id >= 0 && id < OSCData.size()) {
        frameId = id;
    }
}

int Analysis::getFrameId() const
{
    return frameId;
}

// ====================== 主入口 ======================

void Analysis::processPcapData(const QVector<QByteArray>& pcapData)
{
    if (pcapData.isEmpty()) {
        emit statusUpdate("Analysis: Empty pcap data");
        return;
    }

    emit statusUpdate(QString("Analysis: Processing %1 packets...").arg(pcapData.size()));

    handleChannelData(pcapData);
    funcPcap();
}

// ====================== 通道数据解析 ======================

void Analysis::handleChannelData(const QVector<QByteArray>& pcapData)
{
    ch1Data.clear();
    ch2Data.clear();
    ch3Data.clear();
    ch4Data.clear();

    for (const QByteArray& packetData : pcapData) {
        if (packetData.size() != 1005) {
            // 跳过非标准数据包（可能是EOF标记包）
            continue;
        }

        // 解析4通道数据：每通道192字节int8
        QByteArray channel1Data = packetData.mid(0, 192);
        QByteArray channel2Data = packetData.mid(200, 192);
        QByteArray channel3Data = packetData.mid(400, 192);
        QByteArray channel4Data = packetData.mid(600, 192);

        ch1Data.append(channel1Data);
        ch2Data.append(channel2Data);
        ch3Data.append(channel3Data);
        ch4Data.append(channel4Data);
    }

    emit statusUpdate(QString("Analysis: Parsed %1 pkts, ch1=%2B ch2=%3B ch3=%4B ch4=%5B")
        .arg(pcapData.size())
        .arg(ch1Data.size()).arg(ch2Data.size())
        .arg(ch3Data.size()).arg(ch4Data.size()));
}

// ====================== DBI处理管线 ======================

void Analysis::funcPcap()
{
    // LO本振频率: 34.4GHz和32GHz用于下变频
    double lo_freq[ch_num - 1] = { 34.4, 32 };

    if (ch1Data.isEmpty() || ch2Data.isEmpty() || ch3Data.isEmpty() || ch4Data.isEmpty()) {
        qWarning() << "Analysis: ADC channel data is empty!";
        emit statusUpdate("Analysis: Error - ADC channel data is empty");
        return;
    }

    // 计算数据长度
    int len_ch1 = ch1Data.size();
    int len_ch2 = ch2Data.size();
    int len_ch3 = ch4Data.size();  // 使用ch4作为第3通道

    int len_in = len_ch1;
    int num_pkt = len_in / 192;
    int len_out = num_pkt * 192;  // 简化计算

    // 分配内存并转换int8->double
    double* adc_data[ch_num];
    adc_data[0] = (double*)malloc(sizeof(double) * len_ch1);
    adc_data[1] = (double*)malloc(sizeof(double) * len_ch2);
    adc_data[2] = (double*)malloc(sizeof(double) * len_ch3);

    if (!adc_data[0] || !adc_data[1] || !adc_data[2]) {
        qCritical() << "Analysis: Memory allocation failed!";
        emit statusUpdate("Analysis: Error - Memory allocation failed");
        return;
    }

    for (int i = 0; i < len_ch1; ++i) {
        adc_data[0][i] = static_cast<double>(static_cast<qint8>(ch1Data[i]));
    }
    for (int i = 0; i < len_ch2; ++i) {
        adc_data[1][i] = static_cast<double>(static_cast<qint8>(ch2Data[i]));
    }
    for (int i = 0; i < len_ch3; ++i) {
        adc_data[2][i] = static_cast<double>(static_cast<qint8>(ch4Data[i]));
    }

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

    // 转换为float并归一化，保存DBI完整输出（帧分割前）
    m_dbiOutput.clear();
    for (int i = 0; i < len_out; ++i) {
        m_dbiOutput.append(static_cast<float>(output_data[i] / 150.0));
    }

    // 后处理：分段+构建波形数据
    funcADC(m_dbiOutput);

    // 释放内存
    for (int i = 0; i < ch_num; i++) {
        free(adc_data[i]);
        free(w_miso[i]);
    }
    for (int i = 0; i < ch_num - 1; i++) {
        free(ft_after_mixer[i]);
    }
    free(ft_before_mixer);
    free(output_data);
    free(sync_delay);

    emit statusUpdate(QString("Analysis: Complete - %1 frames").arg(OSCData.size()));
}

// ====================== ADC后处理：分段 + 构建波形 ======================

void Analysis::funcADC(const QVector<float>& adcData)
{
    // 每2400点找一次峰值位置（帧同步）
    std::vector<int> maxIndices;

    int count = 0;
    float maxValue = std::numeric_limits<float>::lowest();
    int maxIndex = -1;

    for (int i = 0; i < adcData.size(); ++i) {
        float value = adcData[i];

        if (value > maxValue) {
            maxValue = value;
            maxIndex = count;
        }

        count++;
        if (count % 2400 == 0) {
            maxIndices.push_back(maxIndex);
            maxValue = std::numeric_limits<float>::lowest();
            maxIndex = -1;
        }
    }

    if (count % 2400 != 0) {
        maxIndices.push_back(maxIndex);
    }

    // 频率参数
    float calibratinFreq = 370.0f;  // 默认校准频率
    float samplingFrequency = 120.0f;  // 120GHz
    float timeInterval = std::pow(10.0f, -9) / samplingFrequency;
    float fInterval = static_cast<float>(timeInterval / (2 * M_PI * 4320) * std::pow(10.0, 15));
    freqInterval = fInterval;

    int dLeft = static_cast<int>((calibratinFreq - 200) / fInterval + 1);
    int dRight = static_cast<int>((910 - calibratinFreq) / fInterval + 1);
    int zeroC = static_cast<int>(200 / fInterval);

    QVector<QVector<float>> finalData;
    QVector<float> bu(zeroC, 0.0f);

    // 以各帧峰值为中心截取频谱区间
    for (int m = static_cast<int>(maxIndices.size() * 0.1); m < static_cast<int>(maxIndices.size()); ++m) {
        int lc = maxIndices[m];
        int lLeft = lc + dLeft;
        int lRight = lc - dRight;

        if (lRight < 0 || lLeft > adcData.size() - 1) {
            continue;
        }

        QVector<float> data;
        data.append(bu);

        for (int n = lLeft; n >= lRight; --n) {
            data.append(adcData[n]);
        }

        finalData.append(data);
    }

    // 存储分段数据
    OSCData.clear();
    OSCData.append(finalData);

    if (OSCData.isEmpty()) {
        emit statusUpdate("Analysis: Warning - No valid frames extracted");
        return;
    }

    // 构建首帧的(time, amplitude)对
    waveData.clear();
    float ftime = 0;
    float fDeltatime = fInterval;
    QVector<float> temp = finalData[0];

    for (int var = 0; var < temp.size(); ++var) {
        float fstrn = temp[var];
        waveData.append(std::make_pair(ftime, fstrn));
        ftime += fDeltatime;
    }

    frameId = 0;
    emit waveDataReady();
}

// ====================== 帧切换回放 ======================

void Analysis::refreshCurrentFrame()
{
    if (OSCData.isEmpty() || frameId < 0 || frameId >= OSCData.size()) {
        return;
    }

    waveData.clear();
    float ftime = 0;
    float fDeltatime = freqInterval;
    QVector<float> temp = OSCData[frameId];

    for (int var = 0; var < temp.size(); ++var) {
        float fstrn = temp[var];
        waveData.append(std::make_pair(ftime, fstrn));
        ftime += fDeltatime;
    }

    emit waveDataReady();
}

// ====================== 从CSV加载DBI数据 ======================

// ====================== 从CSV加载已分割帧数据（如NOSIGDATA.csv） ======================

void Analysis::loadOscData(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit statusUpdate("Cannot open OSC data file: " + filePath);
        return;
    }

    OSCData.clear();
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;
        QStringList values = line.split(",");
        QVector<float> frame;
        frame.reserve(values.size());
        for (const QString& v : values) {
            frame.append(v.toFloat());
        }
        if (!frame.isEmpty()) {
            OSCData.append(frame);
        }
    }
    file.close();

    if (OSCData.isEmpty()) {
        emit statusUpdate("OSC data file is empty: " + filePath);
        return;
    }

    // 设置频率参数（与funcADC一致）
    float samplingFrequency = 120.0f;
    float timeInterval = std::pow(10.0f, -9) / samplingFrequency;
    freqInterval = static_cast<float>(timeInterval / (2 * M_PI * 4320) * std::pow(10.0, 15));

    // 构建首帧波形
    frameId = 0;
    waveData.clear();
    float ftime = 0;
    const QVector<float>& frame = OSCData[0];
    for (int i = 0; i < frame.size(); ++i) {
        waveData.append(std::make_pair(ftime, frame[i]));
        ftime += freqInterval;
    }

    emit statusUpdate(QString("Loaded OSC data: %1 frames, %2 pts/frame")
        .arg(OSCData.size()).arg(OSCData.isEmpty() ? 0 : OSCData[0].size()));
    emit waveDataReady();
}

// ====================== 从CSV加载DBI数据 ======================

void Analysis::loadDbiData(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit statusUpdate("Cannot open no-data file: " + filePath);
        return;
    }

    QTextStream in(&file);
    m_dbiOutput.clear();
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (!line.isEmpty()) {
            bool ok = false;
            float val = line.toFloat(&ok);
            if (ok) {
                m_dbiOutput.append(val);
            }
        }
    }
    file.close();

    if (m_dbiOutput.isEmpty()) {
        emit statusUpdate("No-data file is empty: " + filePath);
        return;
    }

    funcADC(m_dbiOutput);
    emit statusUpdate(QString("Loaded baseline: %1 pts, %2 frames")
        .arg(m_dbiOutput.size()).arg(OSCData.size()));
}

// ====================== 清除数据 ======================

void Analysis::clearData()
{
    m_dbiOutput.clear();
    OSCData.clear();
    waveData.clear();
    ch1Data.clear();
    ch2Data.clear();
    ch3Data.clear();
    ch4Data.clear();
    frameId = 0;
}
