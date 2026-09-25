#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

// A blocking, thread-safe queue shared between the producer (job generator)
// and multiple consumers (equipment worker threads).
template <typename T>
class ThreadSafeQueue {
private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<T> queue_;
    bool shutdown_ = false;

public:
    void push(T item) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(item));
        }
        cv_.notify_one();
    }

    // Blocks until an item is available or the queue has been shut down.
    // Returns std::nullopt once shut down and drained -- this is how
    // worker threads know it's time to exit.
    std::optional<T> pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !queue_.empty() || shutdown_; });

        if (queue_.empty() && shutdown_) {
            return std::nullopt;
        }

        T item = std::move(queue_.front());
        queue_.pop();
        return item;
    }

    // Signals that no more items will be pushed. Wakes every waiting
    // consumer so they can drain the remaining items and exit.
    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            shutdown_ = true;
        }
        cv_.notify_all();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }
};
