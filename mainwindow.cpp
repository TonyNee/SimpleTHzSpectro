#include "mainwindow.h"
#include "datahub.h"

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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_isPlaying(false), m_processingNewData(false)
{
    setWindowTitle("SimpleSpectro - THz Single Channel Spectroscope");

    SimpleDataHub& hub = SimpleDataHub::instance();

    m_xyView = new XYView(this);

    // UdpReceiver：独立线程
    m_udpReceiver = new UdpReceiver(nullptr);

    // Analysis：独立线程
    m_analysis = new Analysis(nullptr);
    m_analysisThread = new QThread(this);
    m_analysis->moveToThread(m_analysisThread);
    connect(m_analysisThread, &QThread::finished, m_analysisThread, &QObject::deleteLater);
    m_analysisThread->start();

    // 自动播放定时器
    m_autoPlayTimer = new QTimer(this);
    m_autoPlayTimer->setInterval(83);  // 12 fps
    connect(m_autoPlayTimer, &QTimer::timeout, this, [this]() {
        SimpleDataHub& h = SimpleDataHub::instance();
        int total = h.OSCData.size();
        if (total == 0) return;
        int nextId = (h.frameId + 1) % total;
        h.frameId = nextId;
        m_analysis->refreshCurrentFrame();
    });

    setupUI();

    // ===== 核心信号连接：全部通过 SimpleDataHub 中枢 =====

    // UdpReceiver 状态信号
    connect(m_udpReceiver, &UdpReceiver::statusUpdate,
            this, &MainWindow::onStatusUpdate);
    connect(m_udpReceiver, &UdpReceiver::packetCountChanged,
            this, &MainWindow::onPacketCountChanged);

    // SimpleDataHub → Analysis (跨线程 queued)
    connect(&hub, &SimpleDataHub::pcapDataReady,
            m_analysis, &Analysis::onPcapDataReady);

    // Analysis 状态信号
    connect(m_analysis, &Analysis::statusUpdate,
            this, &MainWindow::onStatusUpdate);

    // SimpleDataHub → MainWindow (波形就绪)
    connect(&hub, &SimpleDataHub::waveDataReady,
            this, &MainWindow::onWaveDataReady);
    connect(&hub, &SimpleDataHub::statusUpdate,
            this, &MainWindow::onStatusUpdate);
    connect(&hub, &SimpleDataHub::packetCountChanged,
            this, &MainWindow::onPacketCountChanged);

    onStatusUpdate("Ready. 1.Bind NET → 2.Trigger ADC → 3.Save CSV");

    // 加载基线数据并自动播放
    m_analysis->loadOscData("Config/nodata.csv");
    m_progressBar->setValue(0);
    m_progressBar->setFormat("Baseline");
    int total = hub.OSCData.size();
    if (total > 1) {
        m_isPlaying = true;
        m_autoPlayTimer->start();
        m_btnStop->setText("Stop");
        m_btnStop->setStyleSheet(
            "QPushButton { background-color: #f44336; color: white; font-weight: bold;"
            " padding: 8px 0px; border-radius: 4px; }");
        m_btnStop->setEnabled(true);
    }

    resize(1250, 900);
}

MainWindow::~MainWindow()
{
    m_udpReceiver->stopWorker();
    m_analysisThread->quit();
    m_analysisThread->wait();
}

void MainWindow::setupUI()
{
    QWidget* central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout* mainLayout = new QVBoxLayout(central);

    // ===== Row 1: 工作流按钮 =====
    QHBoxLayout* topLayout = new QHBoxLayout();

    topLayout->addWidget(new QLabel("Local IP:"));
    m_editBindIP = new QLineEdit("10.10.229.1");
    m_editBindIP->setMaximumWidth(185);
    topLayout->addWidget(m_editBindIP);
    topLayout->addWidget(new QLabel("Port:"));
    m_editBindPort = new QLineEdit("8080");
    m_editBindPort->setMaximumWidth(75);
    topLayout->addWidget(m_editBindPort);
    topLayout->addWidget(new QLabel("ADC IP:"));
    m_editAdcIP = new QLineEdit("10.10.229.11");
    m_editAdcIP->setMaximumWidth(185);
    topLayout->addWidget(m_editAdcIP);
    topLayout->addWidget(new QLabel("ADC Port:"));
    m_editAdcPort = new QLineEdit("5506");
    m_editAdcPort->setMaximumWidth(75);
    topLayout->addWidget(m_editAdcPort);

    m_btnStepBind = new QPushButton("1. Bind NET");
    m_btnStepBind->setFixedWidth(185);
    m_btnStepBind->setStyleSheet(
        "QPushButton { background-color: #2196F3; color: white; font-weight: bold;"
        " padding: 8px 0px; border-radius: 4px; }");
    topLayout->addWidget(m_btnStepBind);

    topLayout->addSpacing(24);
    topLayout->addWidget(new QLabel("Sample(µs):"));
    m_spinSampleTime = new QSpinBox();
    m_spinSampleTime->setRange(1, 100000);
    m_spinSampleTime->setValue(20);
    m_spinSampleTime->setMaximumWidth(85);
    topLayout->addWidget(m_spinSampleTime);

    m_btnStepTrigger = new QPushButton("2. Trigger ADC");
    m_btnStepTrigger->setFixedWidth(185);
    m_btnStepTrigger->setStyleSheet(
        "QPushButton { background-color: #FF9800; color: white; font-weight: bold;"
        " padding: 8px 0px; border-radius: 4px; }");
    topLayout->addWidget(m_btnStepTrigger);

    topLayout->addSpacing(24);
    m_btnStepSave = new QPushButton("3. Save CSV");
    m_btnStepSave->setFixedWidth(185);
    m_btnStepSave->setStyleSheet(
        "QPushButton { background-color: #4CAF50; color: white; font-weight: bold;"
        " padding: 8px 0px; border-radius: 4px; }");
    topLayout->addWidget(m_btnStepSave);

    topLayout->addSpacing(16);
    m_btnClear = new QPushButton("Clear");
    m_btnClear->setFixedWidth(85);
    m_btnClear->setStyleSheet(
        "QPushButton { background-color: #9C27B0; color: white; font-weight: bold;"
        " padding: 8px 0px; border-radius: 4px; }");
    topLayout->addWidget(m_btnClear);

    topLayout->addSpacing(24);
    m_btnPrevFrame = new QPushButton("< Prev"); topLayout->addWidget(m_btnPrevFrame);
    m_btnStop = new QPushButton("Stop");
    m_btnStop->setFixedWidth(85);
    m_btnStop->setEnabled(false);
    m_btnStop->setStyleSheet(
        "QPushButton { background-color: #757575; color: white; font-weight: bold;"
        " padding: 8px 0px; border-radius: 4px; }"
        "QPushButton:enabled { background-color: #f44336; color: white; }");
    topLayout->addWidget(m_btnStop);
    m_btnNextFrame = new QPushButton("Next >"); topLayout->addWidget(m_btnNextFrame);

    topLayout->addStretch();
    topLayout->addWidget(new QLabel("Frame:"));
    m_spinFrameId = new QSpinBox();
    m_spinFrameId->setMinimum(0); m_spinFrameId->setValue(0);
    m_spinFrameId->setMaximumWidth(75);
    topLayout->addWidget(m_spinFrameId);
    m_labelFrameInfo = new QLabel("0 / 0");
    topLayout->addWidget(m_labelFrameInfo);
    mainLayout->addLayout(topLayout);

    // ===== Row 2: Config路径 =====
    QHBoxLayout* pathLayout = new QHBoxLayout();
    QGroupBox* calibGroup = new QGroupBox("Calibration (per-boot)");
    QHBoxLayout* calibPathLayout = new QHBoxLayout(calibGroup);
    m_editCalibDir = new QLineEdit("Config/Calibration");
    m_editCalibDir->setReadOnly(true);
    calibPathLayout->addWidget(m_editCalibDir);
    QPushButton* btnCalibDir = new QPushButton("...");
    btnCalibDir->setMaximumWidth(36);
    calibPathLayout->addWidget(btnCalibDir);
    pathLayout->addWidget(calibGroup);

    QGroupBox* filterGroup = new QGroupBox("DBI Filters (fixed)");
    QHBoxLayout* filterPathLayout = new QHBoxLayout(filterGroup);
    m_editFilterDir = new QLineEdit("Config/Filters");
    m_editFilterDir->setReadOnly(true);
    filterPathLayout->addWidget(m_editFilterDir);
    QPushButton* btnFilterDir = new QPushButton("...");
    btnFilterDir->setMaximumWidth(36);
    filterPathLayout->addWidget(btnFilterDir);
    pathLayout->addWidget(filterGroup);
    mainLayout->addLayout(pathLayout);

    // ===== 中央波形 =====
    mainLayout->addWidget(m_xyView, 1);

    // ===== 进度条 =====
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setFormat("Ready");
    m_progressBar->setMaximumHeight(26);
    m_progressBar->setStyleSheet(
        "QProgressBar { border: 1px solid #444; border-radius: 3px; background: #1a1a2e;"
        " text-align: center; color: #ccc; font-size: 12px; }"
        "QProgressBar::chunk { background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        " stop:0 #2196F3, stop:0.3 #2196F3, stop:0.3 #FF9800,"
        " stop:0.7 #FF9800, stop:0.7 #4CAF50); }");
    mainLayout->addWidget(m_progressBar);

    // ===== 底部日志 =====
    QSplitter* bottomSplitter = new QSplitter(Qt::Horizontal);
    m_labelStatus = new QLabel("Ready");
    m_labelStatus->setStyleSheet("QLabel { color: #888; padding: 2px; }");
    bottomSplitter->addWidget(m_labelStatus);
    m_logView = new QTextEdit();
    m_logView->setReadOnly(true);
    m_logView->setMaximumHeight(160);
    m_logView->setStyleSheet(
        "QTextEdit { background-color: #1a1a2e; color: #aaa; font-size: 13px; }");
    bottomSplitter->addWidget(m_logView);
    bottomSplitter->setStretchFactor(0, 1);
    bottomSplitter->setStretchFactor(1, 3);
    mainLayout->addWidget(bottomSplitter);

    // ===== 信号连接（UI按钮） =====
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
}

// ===== Slot 实现 =====

void MainWindow::onStepBind()
{
    if (m_udpReceiver->isBound()) {
        m_autoPlayTimer->stop();
        m_isPlaying = false;
        m_btnStop->setText("Stop");
        m_btnStop->setStyleSheet(
            "QPushButton { background-color: #757575; color: white; font-weight: bold;"
            " padding: 8px 0px; border-radius: 4px; }");
        m_btnStop->setEnabled(false);
        m_udpReceiver->stopWorker();
        m_btnStepBind->setText("1. Bind NET");
        m_btnStepBind->setStyleSheet(
            "QPushButton { background-color: #2196F3; color: white; font-weight: bold;"
            " padding: 8px 18px; border-radius: 4px; }");
        onStatusUpdate("Unbound.");
    } else {
        QString ip = m_editBindIP->text();
        quint16 port = m_editBindPort->text().toUShort();
        QString adcIp = m_editAdcIP->text();
        quint16 adcPort = m_editAdcPort->text().toUShort();
        m_udpReceiver->startWorker(ip, port, adcIp, adcPort);
        m_btnStepBind->setText("1. Unbind NET");
        m_btnStepBind->setStyleSheet(
            "QPushButton { background-color: #f44336; color: white; font-weight: bold;"
            " padding: 8px 18px; border-radius: 4px; }");
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
    int sampleTimeNs = sampleTimeUs * 1000;
    SimpleDataHub::instance().sampleTimeNs = sampleTimeNs;  // 同步更新hub
    m_udpReceiver->sendADCMessage(sampleTimeNs);            // µs → ns
}

void MainWindow::onStop()
{
    SimpleDataHub& hub = SimpleDataHub::instance();
    int total = hub.OSCData.size();

    if (m_isPlaying) {
        m_autoPlayTimer->stop();
        m_isPlaying = false;
        m_btnStop->setText("Run");
        m_btnStop->setStyleSheet(
            "QPushButton { background-color: #4CAF50; color: white; font-weight: bold;"
            " padding: 8px 0px; border-radius: 4px; }");
        if (total > 0)
            m_labelStatus->setText(QString("[Manual] Frame %1 / %2").arg(hub.frameId).arg(total));
        onStatusUpdate("Paused. Use < Prev / Next > to browse.");
    } else {
        if (total == 0) { onStatusUpdate("No data. Trigger ADC first."); return; }
        m_autoPlayTimer->start();
        m_isPlaying = true;
        m_btnStop->setText("Stop");
        m_btnStop->setStyleSheet(
            "QPushButton { background-color: #f44336; color: white; font-weight: bold;"
            " padding: 8px 0px; border-radius: 4px; }");
        m_labelStatus->setText(QString("[Auto] Frame %1 / %2").arg(hub.frameId).arg(total));
        onStatusUpdate("Auto-play resumed.");
    }
}

void MainWindow::onClear()
{
    m_autoPlayTimer->stop();
    m_isPlaying = false;
    m_analysis->clearData();
    m_xyView->setdata(SimpleDataHub::instance().waveData);
    updateFrameInfo();
    m_analysis->loadOscData("Config/nodata.csv");

    SimpleDataHub& hub = SimpleDataHub::instance();
    int total = hub.OSCData.size();
    if (total > 1) {
        m_isPlaying = true;
        m_autoPlayTimer->start();
        m_btnStop->setText("Stop");
        m_btnStop->setStyleSheet(
            "QPushButton { background-color: #f44336; color: white; font-weight: bold;"
            " padding: 8px 0px; border-radius: 4px; }");
        m_btnStop->setEnabled(true);
    }
    m_progressBar->setValue(0);
    m_progressBar->setFormat("Baseline");
    m_labelStatus->setText("Baseline restored.");
}

void MainWindow::onPacketCountChanged(int count)
{
    m_labelStatus->setText(QString("Sampling... %1 packets").arg(count));
    int pct = std::min(5 + count / 2, 45);
    m_progressBar->setValue(pct);
    m_progressBar->setFormat(QString("Stage 1/2 — Sampling (%1 pkts)").arg(count));
}

void MainWindow::onSaveCsv()
{
    SimpleDataHub& hub = SimpleDataHub::instance();

    // 保存DBI输出数据（参考源程序 write_file(path_output_data, output_data)）
    QVector<float> dbiCopy = hub.dbiOutput;

    if (dbiCopy.isEmpty()) {
        onStatusUpdate("No DBI data to save. Trigger ADC first.");
        return;
    }

    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
                          + "/dbi_output_" + QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss") + ".csv";
    QString fileName = QFileDialog::getSaveFileName(this, "Save DBI Output", defaultPath, "CSV Files (*.csv)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        onStatusUpdate("Cannot open: " + fileName);
        return;
    }
    QTextStream out(&file);
    for (int i = 0; i < dbiCopy.size(); ++i)
        out << QString::number(dbiCopy[i]) << "\n";
    file.close();
    onStatusUpdate(QString("DBI output saved: %1 pts → %2").arg(dbiCopy.size()).arg(fileName));
}

void MainWindow::onPrevFrame()
{
    if (m_isPlaying) { onStatusUpdate("Click Stop first."); return; }
    SimpleDataHub& hub = SimpleDataHub::instance();
    int total = hub.OSCData.size();
    if (total == 0) { onStatusUpdate("No data."); return; }
    hub.frameId = (hub.frameId > 0) ? (hub.frameId - 1) : (total - 1);
    m_analysis->refreshCurrentFrame();
}

void MainWindow::onNextFrame()
{
    if (m_isPlaying) { onStatusUpdate("Click Stop first."); return; }
    SimpleDataHub& hub = SimpleDataHub::instance();
    int total = hub.OSCData.size();
    if (total == 0) { onStatusUpdate("No data."); return; }
    hub.frameId = (hub.frameId < total - 1) ? (hub.frameId + 1) : 0;
    m_analysis->refreshCurrentFrame();
}

void MainWindow::onFrameIdChanged(int id)
{
    if (m_isPlaying) return;
    SimpleDataHub& hub = SimpleDataHub::instance();
    int total = hub.OSCData.size();
    if (total == 0) return;
    if (id >= 0 && id < total) {
        hub.frameId = id;
        m_analysis->refreshCurrentFrame();
    }
}

void MainWindow::onWaveDataReady()
{
    SimpleDataHub& hub = SimpleDataHub::instance();
    m_xyView->setdata(hub.waveData);
    updateFrameInfo();

    int total = hub.OSCData.size();
    if (total > 0)
        m_labelStatus->setText(QString("%1Frame %2 / %3")
            .arg(m_isPlaying ? "[Auto] " : "").arg(hub.frameId).arg(total));

    if (m_processingNewData) {
        m_processingNewData = false;
        m_progressBar->setValue(100);
        m_progressBar->setFormat(QString("Done — %1 frames").arg(total));
        m_isPlaying = true;
        m_autoPlayTimer->start();
        m_btnStop->setText("Stop");
        m_btnStop->setStyleSheet(
            "QPushButton { background-color: #f44336; color: white; font-weight: bold;"
            " padding: 8px 0px; border-radius: 4px; }");
        m_btnStop->setEnabled(true);
    }
}

void MainWindow::onStatusUpdate(const QString& msg)
{
    m_labelStatus->setText(msg);
    m_logView->append(QDateTime::currentDateTime().toString("hh:mm:ss") + " " + msg);
    if (msg.contains("Running DBI")) { m_progressBar->setValue(85); m_progressBar->setFormat("Stage 2/2 — DBI..."); }
    else if (msg.contains("Complete")) { m_progressBar->setValue(90); m_progressBar->setFormat("Segmentation..."); }
}

void MainWindow::onChooseCalibDir()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Calibration Directory", m_editCalibDir->text());
    if (!dir.isEmpty()) { m_editCalibDir->setText(dir); m_analysis->setCalibrationDir(dir); }
}

void MainWindow::onChooseFilterDir()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Filter Directory", m_editFilterDir->text());
    if (!dir.isEmpty()) { m_editFilterDir->setText(dir); m_analysis->setFilterDir(dir); }
}

void MainWindow::updateFrameInfo()
{
    SimpleDataHub& hub = SimpleDataHub::instance();
    int total = hub.OSCData.size();
    m_spinFrameId->blockSignals(true);
    m_spinFrameId->setMaximum(total > 0 ? total - 1 : 0);
    m_spinFrameId->setValue(hub.frameId);
    m_spinFrameId->blockSignals(false);
    m_labelFrameInfo->setText(QString("%1 / %2").arg(hub.frameId).arg(total));
}
