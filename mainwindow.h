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
    void onPacketCountChanged(int count);
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

    // 按钮
    QPushButton* m_btnStepBind;
    QPushButton* m_btnStepTrigger;
    QPushButton* m_btnStop;
    QPushButton* m_btnClear;
    QPushButton* m_btnStepSave;
    QPushButton* m_btnPrevFrame;
    QPushButton* m_btnNextFrame;
    QSpinBox* m_spinFrameId;

    // 自动播放
    QTimer* m_autoPlayTimer;
    bool m_isPlaying;
    bool m_processingNewData;

    // UDP/采样配置
    QLineEdit* m_editBindIP;
    QLineEdit* m_editBindPort;
    QLineEdit* m_editAdcIP;
    QLineEdit* m_editAdcPort;
    QSpinBox* m_spinSampleTime;

    // 路径配置
    QLineEdit* m_editCalibDir;
    QLineEdit* m_editFilterDir;

    // 状态
    QProgressBar* m_progressBar;
    QLabel* m_labelFrameInfo;
    QLabel* m_labelStatus;
    QTextEdit* m_logView;
};
