#include "EquipmentWorker.h"
#include <thread>
#include <chrono>
#include <random>

QString EquipmentWorker::toString(EquipmentState state) {
    switch (state) {
        case EquipmentState::Idle:    return "IDLE";
        case EquipmentState::Running: return "RUNNING";
        case EquipmentState::Alarm:   return "ALARM";
        case EquipmentState::Down:    return "DOWN";
    }
    return "UNKNOWN";
}

EquipmentWorker::EquipmentWorker(int workerId, ThreadSafeQueue<Job>& jobQueue, QObject* parent)
    : QObject(parent), workerId_(workerId), jobQueue_(jobQueue) {}

void EquipmentWorker::setState(EquipmentState newState) {
    state_.store(newState);
    emit stateChanged(workerId_, toString(newState));
}

void EquipmentWorker::processJob(const Job& job) {
    setState(EquipmentState::Running);
    emit logMessage(QString("[Equipment-%1] starting job %2 (lot %3)")
                         .arg(workerId_)
                         .arg(job.id)
                         .arg(QString::fromStdString(job.waferLotId)));

    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> alarmChance(1, 20); // 1-in-20 -> 5%

    std::this_thread::sleep_for(std::chrono::milliseconds(job.processingTimeMs / 2));

    if (alarmChance(rng) == 1) {
        setState(EquipmentState::Alarm);
        emit logMessage(QString("[Equipment-%1] ALARM during job %2, recovering...")
                             .arg(workerId_).arg(job.id));
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(job.processingTimeMs / 2));

    emit logMessage(QString("[Equipment-%1] completed job %2 (lot %3)")
                         .arg(workerId_)
                         .arg(job.id)
                         .arg(QString::fromStdString(job.waferLotId)));
}

void EquipmentWorker::run() {
    setState(EquipmentState::Idle);

    while (true) {
        std::optional<Job> job = jobQueue_.pop();
        if (!job.has_value()) {
            break; // queue shut down and drained
        }
        processJob(*job);
        setState(EquipmentState::Idle);
    }

    setState(EquipmentState::Down);
    emit logMessage(QString("[Equipment-%1] shutting down, no more jobs.").arg(workerId_));
}
