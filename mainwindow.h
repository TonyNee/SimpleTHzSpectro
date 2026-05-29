#pragma once
#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QTextEdit>
#include <QTimer>
#include <QProgressBar>
#include <QThread>

#include "Axis/XYView.h"
#include "udpreceiver.h"
#include "analysis.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void processData(QVector<QByteArray> pcapData);  // 跨线程发送数据到Analysis

private slots:
    void onStepBind();
    void onTriggerAdc();
    void onStop();
    void onClear();
    void onSaveCsv();
    void onPrevFrame();
    void onNextFrame();
    void onFrameIdChanged(int id);
    void onWaveDataReady();
    void onStatusUpdate(const QString& msg);
    void onPacketReceived(int count);
    void onChooseCalibDir();
    void onChooseFilterDir();

private:
    void setupUI();
    void updateFrameInfo();

    // 核心组件
    XYView* m_xyView;
    UdpReceiver* m_udpReceiver;
    Analysis* m_analysis;
    QThread* m_analysisThread;

    // 工作流按钮 (编号 1/2/3)
    QPushButton* m_btnStepBind;
    QPushButton* m_btnStepTrigger;
    QPushButton* m_btnStop;       // 停止自动播放
    QPushButton* m_btnClear;      // 清除数据，恢复无数据基线
    QPushButton* m_btnStepSave;

    // 帧导航
    QPushButton* m_btnPrevFrame;
    QPushButton* m_btnNextFrame;
    QSpinBox* m_spinFrameId;

    // 自动播放
    QTimer* m_autoPlayTimer;
    bool m_isPlaying;
    bool m_processingNewData;  // 标记是否正在处理新采集数据

    // UDP配置
    QLineEdit* m_editBindIP;
    QLineEdit* m_editBindPort;
    QLineEdit* m_editAdcIP;
    QLineEdit* m_editAdcPort;

    // 采样参数
    QSpinBox* m_spinSampleTime;

    // 路径配置
    QLineEdit* m_editCalibDir;
    QLineEdit* m_editFilterDir;

    // 状态显示
    QProgressBar* m_progressBar;
    QLabel* m_labelFrameInfo;
    QLabel* m_labelStatus;
    QTextEdit* m_logView;
};
