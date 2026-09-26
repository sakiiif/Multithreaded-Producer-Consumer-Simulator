#pragma once
#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QPlainTextEdit>
#include <QWidget>
#include <vector>
#include <thread>
#include <memory>
#include "Job.h"
#include "ThreadSafeQueue.h"
#include "EquipmentWorker.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onStartClicked();
    void onWorkerStateChanged(int workerId, const QString& state);
    void onLogMessage(const QString& message);
    void onSimulationFinished();

private:
    void buildWorkerRows(int count);
    void setControlsEnabled(bool enabled);
    static QString colorForState(const QString& state);

    // --- UI ---
    QSpinBox* workerCountSpin_;
    QSpinBox* jobCountSpin_;
    QPushButton* startButton_;
    QPlainTextEdit* logView_;
    QWidget* workerPanel_;
    std::vector<QLabel*> stateLabels_;

    // --- simulation state ---
    std::unique_ptr<ThreadSafeQueue<Job>> jobQueue_;
    std::vector<std::unique_ptr<EquipmentWorker>> workers_;
    std::vector<std::thread> workerThreads_;
    std::thread producerAndJoinThread_;
};
