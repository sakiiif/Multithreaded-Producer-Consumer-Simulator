#pragma once
#include <string>
#include <mutex>

// A tiny thread-safe singleton logger so multiple worker threads can print
// to std::cout without interleaving each other's output.
class Logger {
public:
    static Logger& instance();
    void log(const std::string& message);

private:
    Logger() = default;
    std::mutex mutex_;
};
