#pragma once
#include "Job.h"
#include "ThreadSafeQueue.h"
#include <atomic>
#include <string>

// The states a piece of equipment can be in. This is a simple State
// pattern implemented with an enum + transition function rather than
// separate classes, which is enough for an entry-level project.
enum class EquipmentState {
    Idle,
    Running,
    Alarm,
    Down
};

std::string toString(EquipmentState state);

// Represents one piece of equipment. Each instance runs on its own
// std::thread, pulling jobs off the shared queue and "processing" them.
class EquipmentWorker {
public:
    EquipmentWorker(int workerId, ThreadSafeQueue<Job>& jobQueue);

    // Entry point run on its own std::thread.
    void run();

    EquipmentState state() const { return state_.load(); }

private:
    void setState(EquipmentState newState);
    void processJob(const Job& job);

    int workerId_;
    ThreadSafeQueue<Job>& jobQueue_;
    std::atomic<EquipmentState> state_{EquipmentState::Idle};
};
