#include "EquipmentWorker.h"
#include "Logger.h"
#include <thread>
#include <chrono>
#include <random>
#include <sstream>

std::string toString(EquipmentState state) {
    switch (state) {
        case EquipmentState::Idle:    return "IDLE";
        case EquipmentState::Running: return "RUNNING";
        case EquipmentState::Alarm:   return "ALARM";
        case EquipmentState::Down:    return "DOWN";
    }
    return "UNKNOWN";
}

EquipmentWorker::EquipmentWorker(int workerId, ThreadSafeQueue<Job>& jobQueue)
    : workerId_(workerId), jobQueue_(jobQueue) {}

void EquipmentWorker::setState(EquipmentState newState) {
    state_.store(newState);
    std::ostringstream oss;
    oss << "[Equipment-" << workerId_ << "] state -> " << toString(newState);
    Logger::instance().log(oss.str());
}

void EquipmentWorker::processJob(const Job& job) {
    setState(EquipmentState::Running);

    // Small chance of a mid-job alarm, like a real equipment fault.
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> alarmChance(1, 20); // 1-in-20 -> 5%

    std::this_thread::sleep_for(std::chrono::milliseconds(job.processingTimeMs / 2));

    if (alarmChance(rng) == 1) {
        setState(EquipmentState::Alarm);
        std::ostringstream oss;
        oss << "[Equipment-" << workerId_ << "] ALARM during job " << job.id
             << " (lot " << job.waferLotId << "), recovering...";
        Logger::instance().log(oss.str());
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(job.processingTimeMs / 2));

    std::ostringstream done;
    done << "[Equipment-" << workerId_ << "] completed job " << job.id
          << " (lot " << job.waferLotId << ")";
    Logger::instance().log(done.str());
}

void EquipmentWorker::run() {
    setState(EquipmentState::Idle);

    while (true) {
        std::optional<Job> job = jobQueue_.pop();
        if (!job.has_value()) {
            // Queue was shut down and fully drained -- no more work coming.
            break;
        }
        processJob(*job);
        setState(EquipmentState::Idle);
    }

    setState(EquipmentState::Down);
    std::ostringstream oss;
    oss << "[Equipment-" << workerId_ << "] shutting down, no more jobs.";
    Logger::instance().log(oss.str());
}
