#include "mainwindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QStandardPaths>
#include <QApplication>
#include <QSplitter>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_isPlaying(false)
{
    setWindowTitle("SimpleSpectro - THz Single Channel Spectroscope");

    m_xyView = new XYView(this);
    m_udpReceiver = new UdpReceiver(this);
    m_analysis = new Analysis(this);

    m_autoPlayTimer = new QTimer(this);
    m_autoPlayTimer->setInterval(83);  // 12 fps
    connect(m_autoPlayTimer, &QTimer::timeout, this, [this]() {
        int total = m_analysis->getFrameCount();
        if (total == 0) return;
        int nextId = (m_analysis->getFrameId() + 1) % total;
        m_analysis->setFrameId(nextId);
        m_analysis->refreshCurrentFrame();
    });

    setupUI();

    // UDP data -> Analysis -> XYView
    connect(m_udpReceiver, &UdpReceiver::dataReady, this, [this]() {
        m_progressBar->setValue(50);
        m_progressBar->setFormat("Stage 2/2 — DBI Processing...");
        QApplication::processEvents();

        m_analysis->processPcapData(m_udpReceiver->getPcapData());
        m_udpReceiver->clearBuffer();

        m_progressBar->setValue(90);
        m_progressBar->setFormat("Stage 2/2 — Segmentation...");

        // 新数据到达，启动自动播放
        m_isPlaying = true;
        m_autoPlayTimer->start();
        m_btnStop->setText("Stop");
        m_btnStop->setStyleSheet(
            "QPushButton { background-color: #f44336; color: white; font-weight: bold;"
            " padding: 6px 0px; border-radius: 4px; }");
        m_btnStop->setEnabled(true);
    });

    connect(m_analysis, &Analysis::waveDataReady, this, &MainWindow::onWaveDataReady);
    connect(m_analysis, &Analysis::statusUpdate, this, &MainWindow::onStatusUpdate);
    connect(m_udpReceiver, &UdpReceiver::statusUpdate, this, &MainWindow::onStatusUpdate);

    onStatusUpdate("Ready. 1.Bind NET -> 2.Trigger ADC -> 3.Save CSV");

    // 加载无数据基线并自动开始播放
    m_analysis->loadOscData("Config/nodata.csv");
    m_progressBar->setValue(0);
    m_progressBar->setFormat("Baseline");
    if (m_analysis->getFrameCount() > 1) {
        m_isPlaying = true;
        m_autoPlayTimer->start();
        m_btnStop->setText("Stop");
        m_btnStop->setStyleSheet(
            "QPushButton { background-color: #f44336; color: white; font-weight: bold;"
            " padding: 6px 0px; border-radius: 4px; }");
        m_btnStop->setEnabled(true);
    }

    resize(1100, 700);
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    QWidget* central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout* mainLayout = new QVBoxLayout(central);

    // ===== Row 1: Workflow =====
    QHBoxLayout* topLayout = new QHBoxLayout();

    // UDP config
    topLayout->addWidget(new QLabel("Local IP:"));
    m_editBindIP = new QLineEdit("10.10.229.1");
    m_editBindIP->setMaximumWidth(110);
    topLayout->addWidget(m_editBindIP);
    topLayout->addWidget(new QLabel("Port:"));
    m_editBindPort = new QLineEdit("8080");
    m_editBindPort->setMaximumWidth(60);
    topLayout->addWidget(m_editBindPort);
    topLayout->addWidget(new QLabel("ADC IP:"));
    m_editAdcIP = new QLineEdit("10.10.229.11");
    m_editAdcIP->setMaximumWidth(110);
    topLayout->addWidget(m_editAdcIP);
    topLayout->addWidget(new QLabel("ADC Port:"));
    m_editAdcPort = new QLineEdit("5506");
    m_editAdcPort->setMaximumWidth(60);
    topLayout->addWidget(m_editAdcPort);

    // Step 1: Bind
    m_btnStepBind = new QPushButton("1. Bind NET");
    m_btnStepBind->setFixedWidth(140);
    m_btnStepBind->setStyleSheet(
        "QPushButton { background-color: #2196F3; color: white; font-weight: bold;"
        " padding: 6px 0px; border-radius: 4px; }");
    topLayout->addWidget(m_btnStepBind);

    topLayout->addSpacing(20);

    // Sample time + Step 2: Trigger ADC
    topLayout->addWidget(new QLabel("Sample(µs):"));
    m_spinSampleTime = new QSpinBox();
    m_spinSampleTime->setRange(1, 100000);
    m_spinSampleTime->setValue(5);
    m_spinSampleTime->setMaximumWidth(70);
    topLayout->addWidget(m_spinSampleTime);

    m_btnStepTrigger = new QPushButton("2. Trigger ADC");
    m_btnStepTrigger->setFixedWidth(140);
    m_btnStepTrigger->setStyleSheet(
        "QPushButton { background-color: #FF9800; color: white; font-weight: bold;"
        " padding: 6px 0px; border-radius: 4px; }");
    topLayout->addWidget(m_btnStepTrigger);

    topLayout->addSpacing(20);

    // Step 3: Save CSV
    m_btnStepSave = new QPushButton("3. Save CSV");
    m_btnStepSave->setFixedWidth(140);
    m_btnStepSave->setStyleSheet(
        "QPushButton { background-color: #4CAF50; color: white; font-weight: bold;"
        " padding: 6px 0px; border-radius: 4px; }");
    topLayout->addWidget(m_btnStepSave);

    topLayout->addSpacing(12);

    // Clear data
    m_btnClear = new QPushButton("Clear");
    m_btnClear->setFixedWidth(70);
    m_btnClear->setStyleSheet(
        "QPushButton { background-color: #9C27B0; color: white; font-weight: bold;"
        " padding: 6px 0px; border-radius: 4px; }");
    topLayout->addWidget(m_btnClear);

    topLayout->addSpacing(20);

    // Frame nav: Prev + Stop + Next
    m_btnPrevFrame = new QPushButton("< Prev");
    topLayout->addWidget(m_btnPrevFrame);

    m_btnStop = new QPushButton("Stop");
    m_btnStop->setFixedWidth(70);
    m_btnStop->setEnabled(false);
    m_btnStop->setStyleSheet(
        "QPushButton { background-color: #757575; color: white; font-weight: bold;"
        " padding: 6px 0px; border-radius: 4px; }"
        "QPushButton:enabled { background-color: #f44336; color: white; }");
    topLayout->addWidget(m_btnStop);

    m_btnNextFrame = new QPushButton("Next >");
    topLayout->addWidget(m_btnNextFrame);

    topLayout->addStretch();

    // Frame info (far right)
    topLayout->addWidget(new QLabel("Frame:"));
    m_spinFrameId = new QSpinBox();
    m_spinFrameId->setMinimum(0);
    m_spinFrameId->setValue(0);
    m_spinFrameId->setMaximumWidth(60);
    topLayout->addWidget(m_spinFrameId);
    m_labelFrameInfo = new QLabel("0 / 0");
    topLayout->addWidget(m_labelFrameInfo);

    mainLayout->addLayout(topLayout);

    // ===== Row 2: Config paths =====
    QHBoxLayout* pathLayout = new QHBoxLayout();

    QGroupBox* calibGroup = new QGroupBox("Calibration (per-boot)");
    QHBoxLayout* calibPathLayout = new QHBoxLayout(calibGroup);
    m_editCalibDir = new QLineEdit("Config/Calibration");
    m_editCalibDir->setReadOnly(true);
    calibPathLayout->addWidget(m_editCalibDir);
    QPushButton* btnCalibDir = new QPushButton("...");
    btnCalibDir->setMaximumWidth(30);
    calibPathLayout->addWidget(btnCalibDir);
    pathLayout->addWidget(calibGroup);

    QGroupBox* filterGroup = new QGroupBox("DBI Filters (fixed)");
    QHBoxLayout* filterPathLayout = new QHBoxLayout(filterGroup);
    m_editFilterDir = new QLineEdit("Config/Filters");
    m_editFilterDir->setReadOnly(true);
    filterPathLayout->addWidget(m_editFilterDir);
    QPushButton* btnFilterDir = new QPushButton("...");
    btnFilterDir->setMaximumWidth(30);
    filterPathLayout->addWidget(btnFilterDir);
    pathLayout->addWidget(filterGroup);

    mainLayout->addLayout(pathLayout);

    // ===== Central display =====
    mainLayout->addWidget(m_xyView, 1);

    // ===== Pipeline progress =====
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setFormat("Ready");
    m_progressBar->setMaximumHeight(20);
    m_progressBar->setStyleSheet(
        "QProgressBar { border: 1px solid #444; border-radius: 3px; background: #1a1a2e;"
        " text-align: center; color: #ccc; font-size: 12px; }"
        "QProgressBar::chunk { background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        " stop:0 #2196F3, stop:0.3 #2196F3, stop:0.3 #FF9800, stop:0.7 #FF9800, stop:0.7 #4CAF50); }");
    mainLayout->addWidget(m_progressBar);

    // ===== Bottom status =====
    QSplitter* bottomSplitter = new QSplitter(Qt::Horizontal);

    m_labelStatus = new QLabel("Ready");
    m_labelStatus->setStyleSheet("QLabel { color: #888; padding: 2px; }");
    bottomSplitter->addWidget(m_labelStatus);

    m_logView = new QTextEdit();
    m_logView->setReadOnly(true);
    m_logView->setMaximumHeight(120);
    m_logView->setStyleSheet(
        "QTextEdit { background-color: #1a1a2e; color: #aaa; font-size: 13px; }");
    bottomSplitter->addWidget(m_logView);

    bottomSplitter->setStretchFactor(0, 1);
    bottomSplitter->setStretchFactor(1, 3);
    mainLayout->addWidget(bottomSplitter);

    // ===== Signal connections =====
    connect(m_btnStepBind, &QPushButton::clicked, this, &MainWindow::onStepBind);
    connect(m_btnStepTrigger, &QPushButton::clicked, this, &MainWindow::onTriggerAdc);
    connect(m_btnStop, &QPushButton::clicked, this, &MainWindow::onStop);
    connect(m_btnClear, &QPushButton::clicked, this, &MainWindow::onClear);
    connect(m_btnStepSave, &QPushButton::clicked, this, &MainWindow::onSaveCsv);
    connect(m_btnPrevFrame, &QPushButton::clicked, this, &MainWindow::onPrevFrame);
    connect(m_btnNextFrame, &QPushButton::clicked, this, &MainWindow::onNextFrame);
    connect(m_spinFrameId, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onFrameIdChanged);
    connect(btnCalibDir, &QPushButton::clicked, this, &MainWindow::onChooseCalibDir);
    connect(btnFilterDir, &QPushButton::clicked, this, &MainWindow::onChooseFilterDir);
    connect(m_udpReceiver, &UdpReceiver::packetReceived,
            this, &MainWindow::onPacketReceived);
}

// ===== Slot implementations =====

void MainWindow::onStepBind()
{
    if (m_udpReceiver->isBound()) {
        // 停止自动播放
        m_autoPlayTimer->stop();
        m_isPlaying = false;
        m_btnStop->setText("Stop");
        m_btnStop->setStyleSheet(
            "QPushButton { background-color: #757575; color: white; font-weight: bold;"
            " padding: 6px 0px; border-radius: 4px; }");
        m_btnStop->setEnabled(false);
        m_udpReceiver->unbind();
        m_btnStepBind->setText("1. Bind NET");
        m_btnStepBind->setStyleSheet(
            "QPushButton { background-color: #2196F3; color: white; font-weight: bold;"
            " padding: 6px 16px; border-radius: 4px; }");
        onStatusUpdate("Unbound.");
    } else {
        QString adcIp = m_editAdcIP->text();
        quint16 adcPort = m_editAdcPort->text().toUShort();
        m_udpReceiver->setAdcTarget(adcIp, adcPort);

        QString ip = m_editBindIP->text();
        quint16 port = m_editBindPort->text().toUShort();
        bool ok = m_udpReceiver->bind(ip, port);
        if (ok) {
            m_btnStepBind->setText("1. Unbind NET");
            m_btnStepBind->setStyleSheet(
                "QPushButton { background-color: #f44336; color: white; font-weight: bold;"
                " padding: 6px 16px; border-radius: 4px; }");
        }
    }
}

void MainWindow::onTriggerAdc()
{
    if (!m_udpReceiver->isBound()) {
        onStatusUpdate("Error: Not bound. Click 1.Bind first.");
        return;
    }
    m_progressBar->setValue(0);
    m_progressBar->setFormat("Waiting for data...");
    int sampleTimeUs = m_spinSampleTime->value();
    m_udpReceiver->sendADCMessage(sampleTimeUs * 1000);  // µs -> ns
}

void MainWindow::onStop()
{
    if (m_isPlaying) {
        // 暂停 → 手动模式
        m_autoPlayTimer->stop();
        m_isPlaying = false;
        m_btnStop->setText("Run");
        m_btnStop->setStyleSheet(
            "QPushButton { background-color: #4CAF50; color: white; font-weight: bold;"
            " padding: 6px 0px; border-radius: 4px; }");
        int total = m_analysis->getFrameCount();
        if (total > 0) {
            m_labelStatus->setText(QString("[Manual] Frame %1 / %2")
                .arg(m_analysis->getFrameId()).arg(total));
        }
        onStatusUpdate("Paused. Use < Prev / Next > to browse, Run to resume.");
    } else {
        // 恢复播放
        int total = m_analysis->getFrameCount();
        if (total == 0) {
            onStatusUpdate("No data. Trigger ADC first.");
            return;
        }
        m_autoPlayTimer->start();
        m_isPlaying = true;
        m_btnStop->setText("Stop");
        m_btnStop->setStyleSheet(
            "QPushButton { background-color: #f44336; color: white; font-weight: bold;"
            " padding: 6px 0px; border-radius: 4px; }");
        m_labelStatus->setText(QString("[Auto] Frame %1 / %2")
            .arg(m_analysis->getFrameId()).arg(total));
        onStatusUpdate("Auto-play resumed.");
    }
}

void MainWindow::onClear()
{
    m_autoPlayTimer->stop();
    m_isPlaying = false;
    m_analysis->clearData();
    m_xyView->setdata(m_analysis->getWaveData());
    updateFrameInfo();

    // 重新加载基线并启动播放
    m_analysis->loadOscData("Config/nodata.csv");
    if (m_analysis->getFrameCount() > 1) {
        m_isPlaying = true;
        m_autoPlayTimer->start();
        m_btnStop->setText("Stop");
        m_btnStop->setStyleSheet(
            "QPushButton { background-color: #f44336; color: white; font-weight: bold;"
            " padding: 6px 0px; border-radius: 4px; }");
        m_btnStop->setEnabled(true);
    }
    m_progressBar->setValue(0);
    m_progressBar->setFormat("Baseline");
    m_labelStatus->setText("Baseline restored.");
}

void MainWindow::onPacketReceived(int count)
{
    m_labelStatus->setText(QString("Sampling... %1 packets").arg(count));
    int pct = std::min(5 + count / 2, 45);  // 5-45% during sampling
    m_progressBar->setValue(pct);
    m_progressBar->setFormat(QString("Stage 1/2 — Sampling (%1 pkts)").arg(count));
}

void MainWindow::onSaveCsv()
{
    const QVector<float>& dbiOutput = m_analysis->getDbiOutput();
    if (dbiOutput.isEmpty()) {
        onStatusUpdate("No DBI data to save. Trigger ADC first.");
        return;
    }

    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                          + "/DBI_Output_" + QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss") + ".csv";
    QString fileName = QFileDialog::getSaveFileName(this, "Save DBI Output CSV", defaultPath, "CSV Files (*.csv)");

    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        onStatusUpdate("Cannot open file for writing: " + fileName);
        return;
    }

    QTextStream out(&file);

    // 保存DBI处理后的完整数据（帧分割前），每行一个值
    for (int i = 0; i < dbiOutput.size(); ++i) {
        out << QString::number(dbiOutput[i]) << "\n";
    }

    file.close();
    onStatusUpdate(QString("DBI output saved (%1 points) to %2").arg(dbiOutput.size()).arg(fileName));
}

void MainWindow::onPrevFrame()
{
    if (m_isPlaying) {
        onStatusUpdate("Click Stop first to browse frames manually.");
        return;
    }
    int total = m_analysis->getFrameCount();
    if (total == 0) {
        onStatusUpdate("No data. Trigger ADC first.");
        return;
    }
    int currId = m_analysis->getFrameId();
    int newId = (currId > 0) ? (currId - 1) : (total - 1);
    m_analysis->setFrameId(newId);
    m_analysis->refreshCurrentFrame();
}

void MainWindow::onNextFrame()
{
    if (m_isPlaying) {
        onStatusUpdate("Click Stop first to browse frames manually.");
        return;
    }
    int total = m_analysis->getFrameCount();
    if (total == 0) {
        onStatusUpdate("No data. Trigger ADC first.");
        return;
    }
    int currId = m_analysis->getFrameId();
    int newId = (currId < total - 1) ? (currId + 1) : 0;
    m_analysis->setFrameId(newId);
    m_analysis->refreshCurrentFrame();
}

void MainWindow::onFrameIdChanged(int id)
{
    if (m_isPlaying) {
        return;
    }
    int maxFrames = m_analysis->getFrameCount();
    if (maxFrames == 0) {
        return;
    }
    if (id >= 0 && id < maxFrames) {
        m_analysis->setFrameId(id);
        m_analysis->refreshCurrentFrame();
    }
}

void MainWindow::onWaveDataReady()
{
    m_xyView->setdata(m_analysis->getWaveData());
    updateFrameInfo();

    int total = m_analysis->getFrameCount();
    if (total > 0) {
        m_labelStatus->setText(QString("%1Frame %2 / %3")
            .arg(m_isPlaying ? "[Auto] " : "")
            .arg(m_analysis->getFrameId()).arg(total));
    }

    m_progressBar->setValue(100);
    m_progressBar->setFormat(QString("Acquisition Done — %1 frames").arg(total));
}

void MainWindow::onStatusUpdate(const QString& msg)
{
    m_labelStatus->setText(msg);
    m_logView->append(QDateTime::currentDateTime().toString("hh:mm:ss") + " " + msg);

    // 根据状态消息更新进度条
    if (msg.contains("Running DBI pipeline")) {
        m_progressBar->setValue(70);
        m_progressBar->setFormat("Stage 2/2 — DBI Pipeline...");
    } else if (msg.contains("Complete")) {
        m_progressBar->setValue(90);
        m_progressBar->setFormat("Stage 2/2 — Segmentation...");
    }
}

void MainWindow::onChooseCalibDir()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Choose Calibration Directory",
                                                     m_editCalibDir->text());
    if (!dir.isEmpty()) {
        m_editCalibDir->setText(dir);
        m_analysis->setCalibrationDir(dir);
        onStatusUpdate("Calibration dir set to: " + dir);
    }
}

void MainWindow::onChooseFilterDir()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Choose DBI Filter Directory",
                                                     m_editFilterDir->text());
    if (!dir.isEmpty()) {
        m_editFilterDir->setText(dir);
        m_analysis->setFilterDir(dir);
        onStatusUpdate("Filter dir set to: " + dir);
    }
}

void MainWindow::updateFrameInfo()
{
    int currId = m_analysis->getFrameId();
    int total = m_analysis->getFrameCount();
    m_spinFrameId->blockSignals(true);
    m_spinFrameId->setMaximum(total > 0 ? total - 1 : 0);
    m_spinFrameId->setValue(currId);
    m_spinFrameId->blockSignals(false);
    m_labelFrameInfo->setText(QString("%1 / %2").arg(currId).arg(total));
}
