#include "Logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

void Logger::log(const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                  now.time_since_epoch()) % 1000;
    auto t = std::chrono::system_clock::to_time_t(now);

    std::tm tm_buf{};
#if defined(_WIN32)
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif

    std::cout << "[" << std::put_time(&tm_buf, "%H:%M:%S")
               << "." << std::setfill('0') << std::setw(3) << ms.count()
               << "] " << message << std::endl;
}
