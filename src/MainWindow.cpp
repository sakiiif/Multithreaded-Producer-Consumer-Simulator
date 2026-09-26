#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <random>
#include <sstream>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Equipment Control Simulator (Qt)");
    resize(620, 520);

    auto* central = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(central);

    // --- controls row ---
    auto* controlsLayout = new QHBoxLayout();
    controlsLayout->addWidget(new QLabel("Workers:"));
    workerCountSpin_ = new QSpinBox();
    workerCountSpin_->setRange(1, 10);
    workerCountSpin_->setValue(3);
    controlsLayout->addWidget(workerCountSpin_);

    controlsLayout->addWidget(new QLabel("Jobs:"));
    jobCountSpin_ = new QSpinBox();
    jobCountSpin_->setRange(1, 100);
    jobCountSpin_->setValue(15);
    controlsLayout->addWidget(jobCountSpin_);

    startButton_ = new QPushButton("Start Simulation");
    connect(startButton_, &QPushButton::clicked, this, &MainWindow::onStartClicked);
    controlsLayout->addWidget(startButton_);
    controlsLayout->addStretch();

    mainLayout->addLayout(controlsLayout);

    // --- worker status panel (populated once Start is clicked) ---
    workerPanel_ = new QWidget();
    workerPanel_->setLayout(new QVBoxLayout());
    mainLayout->addWidget(workerPanel_);

    // --- scrolling log ---
    logView_ = new QPlainTextEdit();
    logView_->setReadOnly(true);
    mainLayout->addWidget(logView_, 1);

    setCentralWidget(central);
}

MainWindow::~MainWindow() {
    // Let an in-flight simulation finish rather than tearing threads down
    // mid-run; simple and safe for a project this size.
    if (producerAndJoinThread_.joinable()) {
        producerAndJoinThread_.join();
    }
}

QString MainWindow::colorForState(const QString& state) {
    if (state == "RUNNING") return "#2ecc71"; // green
    if (state == "ALARM")   return "#e74c3c"; // red
    if (state == "DOWN")    return "#7f8c8d"; // gray
    return "#f1c40f";                          // IDLE -> yellow
}

void MainWindow::buildWorkerRows(int count) {
    // Clear rows left over from a previous run.
    QLayoutItem* item;
    while ((item = workerPanel_->layout()->takeAt(0)) != nullptr) {
        if (QLayout* childLayout = item->layout()) {
            QLayoutItem* child;
            while ((child = childLayout->takeAt(0)) != nullptr) {
                delete child->widget();
                delete child;
            }
        }
        delete item;
    }
    stateLabels_.clear();

    auto* panelLayout = static_cast<QVBoxLayout*>(workerPanel_->layout());

    for (int i = 1; i <= count; ++i) {
        auto* row = new QHBoxLayout();
        auto* nameLabel = new QLabel(QString("Equipment %1:").arg(i));
        auto* stateLabel = new QLabel("IDLE");
        stateLabel->setStyleSheet(QString("background-color:%1; padding:4px; border-radius:4px;")
                                       .arg(colorForState("IDLE")));
        stateLabel->setFixedWidth(100);
        stateLabel->setAlignment(Qt::AlignCenter);

        row->addWidget(nameLabel);
        row->addWidget(stateLabel);
        row->addStretch();

        panelLayout->addLayout(row);
        stateLabels_.push_back(stateLabel);
    }
}

void MainWindow::setControlsEnabled(bool enabled) {
    startButton_->setEnabled(enabled);
    workerCountSpin_->setEnabled(enabled);
    jobCountSpin_->setEnabled(enabled);
}

void MainWindow::onStartClicked() {
    // Clean up the previous run's background thread, if any (it will have
    // already finished, since the Start button is disabled while running).
    if (producerAndJoinThread_.joinable()) {
        producerAndJoinThread_.join();
    }

    const int numWorkers = workerCountSpin_->value();
    const int numJobs = jobCountSpin_->value();

    logView_->clear();
    buildWorkerRows(numWorkers);
    setControlsEnabled(false);

    jobQueue_ = std::make_unique<ThreadSafeQueue<Job>>();
    workers_.clear();
    workerThreads_.clear();

    for (int i = 1; i <= numWorkers; ++i) {
        auto worker = std::make_unique<EquipmentWorker>(i, *jobQueue_);
        connect(worker.get(), &EquipmentWorker::stateChanged,
                this, &MainWindow::onWorkerStateChanged);
        connect(worker.get(), &EquipmentWorker::logMessage,
                this, &MainWindow::onLogMessage);
        workers_.push_back(std::move(worker));
    }

    for (auto& w : workers_) {
        workerThreads_.emplace_back(&EquipmentWorker::run, w.get());
    }

    ThreadSafeQueue<Job>* queuePtr = jobQueue_.get();

    // One background thread plays producer, then waits for every worker to
    // finish, then hands control back to the GUI thread. This keeps the
    // window responsive for the whole simulation.
    producerAndJoinThread_ = std::thread([this, queuePtr, numJobs]() {
        std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> procTime(200, 800);

        for (int jobId = 1; jobId <= numJobs; ++jobId) {
            std::ostringstream lot;
            lot << "LOT-" << (1000 + jobId);
            Job job(jobId, lot.str(), procTime(rng));
            queuePtr->push(job);

            QString msg = QString("[Generator] queued job %1 (%2)")
                               .arg(job.id)
                               .arg(QString::fromStdString(job.waferLotId));
            QMetaObject::invokeMethod(this, [this, msg]() { onLogMessage(msg); },
                                       Qt::QueuedConnection);

            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        }

        queuePtr->shutdown();

        for (auto& t : workerThreads_) {
            t.join();
        }

        QMetaObject::invokeMethod(this, [this]() { onSimulationFinished(); },
                                   Qt::QueuedConnection);
    });
}

void MainWindow::onWorkerStateChanged(int workerId, const QString& state) {
    if (workerId >= 1 && workerId <= static_cast<int>(stateLabels_.size())) {
        auto* label = stateLabels_[workerId - 1];
        label->setText(state);
        label->setStyleSheet(QString("background-color:%1; padding:4px; border-radius:4px;")
                                  .arg(colorForState(state)));
    }
}

void MainWindow::onLogMessage(const QString& message) {
    logView_->appendPlainText(QDateTime::currentDateTime().toString("[HH:mm:ss] ") + message);
}

void MainWindow::onSimulationFinished() {
    setControlsEnabled(true);
    logView_->appendPlainText("=== Simulation finished ===");
}
