#pragma once
#include <string>
#include <utility>

// Represents a single unit of work an equipment tool must process,
// e.g. one wafer lot moving through the machine.
struct Job {
    int id;
    std::string waferLotId;
    int processingTimeMs; // simulated processing duration

    Job(int id_, std::string lot, int procTime)
        : id(id_), waferLotId(std::move(lot)), processingTimeMs(procTime) {}
};
