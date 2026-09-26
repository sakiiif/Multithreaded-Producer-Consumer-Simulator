#pragma once
#include "Job.h"
#include "ThreadSafeQueue.h"
#include <QObject>
#include <QString>
#include <atomic>

// Same state machine idea as the console version, but this worker is a
// QObject so it can emit signals. It still runs on a plain std::thread --
// Qt automatically delivers its signals to the GUI thread as queued
// (thread-safe) calls, because the receiving MainWindow lives on the
// main/GUI thread while these signals are emitted from a background thread.
enum class EquipmentState { Idle, Running, Alarm, Down };

class EquipmentWorker : public QObject {
    Q_OBJECT
public:
    EquipmentWorker(int workerId, ThreadSafeQueue<Job>& jobQueue, QObject* parent = nullptr);

    // Consumer loop. Call this on a std::thread -- never call it directly
    // on the GUI thread, or the window will freeze until it's done.
    void run();

signals:
    void stateChanged(int workerId, const QString& state);
    void logMessage(const QString& message);

private:
    void setState(EquipmentState newState);
    void processJob(const Job& job);
    static QString toString(EquipmentState state);

    int workerId_;
    ThreadSafeQueue<Job>& jobQueue_;
    std::atomic<EquipmentState> state_{EquipmentState::Idle};
};
