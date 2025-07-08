#pragma once
#include <memory>
#include <mutex>
#include <queue>

template <typename MessageType>
class Subscriber {
protected:
    std::queue<std::shared_ptr<MessageType>> msgQueue;
    std::mutex queueLock;

public:
    virtual void enqueue(std::shared_ptr<MessageType> message) {
        std::lock_guard<std::mutex> lock(queueLock);
        msgQueue.push(message);
    }
};
