#include "Job.h"
#include "ThreadSafeQueue.h"
#include "EquipmentWorker.h"
#include "Logger.h"

#include <thread>
#include <vector>
#include <memory>
#include <random>
#include <sstream>

int main(int argc, char* argv[]) {
    const int numWorkers = (argc > 1) ? std::stoi(argv[1]) : 3;
    const int numJobs    = (argc > 2) ? std::stoi(argv[2]) : 15;

    Logger::instance().log("=== Equipment Control Simulator starting ===");
    {
        std::ostringstream oss;
        oss << "Workers: " << numWorkers << ", Jobs: " << numJobs;
        Logger::instance().log(oss.str());
    }

    ThreadSafeQueue<Job> jobQueue;

    // --- Spawn equipment worker threads (consumers) ---
    std::vector<std::unique_ptr<EquipmentWorker>> workers;
    std::vector<std::thread> workerThreads;

    for (int i = 1; i <= numWorkers; ++i) {
        workers.push_back(std::make_unique<EquipmentWorker>(i, jobQueue));
        workerThreads.emplace_back(&EquipmentWorker::run, workers.back().get());
    }

    // --- Job generator (producer) runs on the main thread ---
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> procTime(200, 800); // ms

    for (int jobId = 1; jobId <= numJobs; ++jobId) {
        std::ostringstream lot;
        lot << "LOT-" << (1000 + jobId);

        Job job(jobId, lot.str(), procTime(rng));
        jobQueue.push(job);

        std::ostringstream oss;
        oss << "[Generator] queued job " << job.id << " (" << job.waferLotId << ")";
        Logger::instance().log(oss.str());

        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }

    // No more jobs coming: signal shutdown so workers exit once drained.
    jobQueue.shutdown();

    for (auto& t : workerThreads) {
        t.join();
    }

    Logger::instance().log("=== All jobs processed. Simulator stopped. ===");
    return 0;
}
