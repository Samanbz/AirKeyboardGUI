#pragma once

#include <memory>
#include <mutex>
#include <queue>

#include "Subscriber.h"

template <typename MessageType, size_t batchSize>
class BatchSubscriber : public Subscriber<MessageType> {
protected:
    std::queue<std::shared_ptr<MessageType>> flushQueue;
    std::condition_variable cv;

    virtual void processBatch() = 0;

public:
    bool waitForBatch(std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(this->queueLock);
        return cv.wait_for(lock, timeout, [this] {
            return this->msgQueue.size() >= batchSize;
        });
    }

    void flush() {
        {
            std::lock_guard<std::mutex> lock(this->queueLock);
            this->msgQueue.swap(flushQueue);
        }

        if (!flushQueue.empty()) {
            processBatch();
            // Clear the flush queue after processing
            while (!flushQueue.empty()) flushQueue.pop();
        }
    }

    void enqueue(std::shared_ptr<MessageType> message) override {
        bool shouldNotify = false;
        {
            std::lock_guard<std::mutex> lock(this->queueLock);
            this->msgQueue.push(message);
            shouldNotify = (this->msgQueue.size() >= batchSize);
        }

        if (shouldNotify) {
            cv.notify_one();
        }
    }

    ~BatchSubscriber() {
        flush();  // Ensure any remaining messages are processed before destruction
    }
};