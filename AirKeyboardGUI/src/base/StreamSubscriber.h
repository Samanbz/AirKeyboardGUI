#pragma once

#include "Subscriber.h"

template <typename MessageType>
class StreamSubscriber : public Subscriber<MessageType> {
protected:
    virtual void update(std::shared_ptr<MessageType> message) = 0;

public:
    void dequeue() {
        std::shared_ptr<MessageType> message;
        {
            std::lock_guard<std::mutex> lock(this->queueLock);
            if (!this->msgQueue.empty()) {
                message = this->msgQueue.front();
                this->msgQueue.pop();
            }
        }  // Lock released here

        if (message) {
            update(message);  // Safe: no lock held
        }
    }
};
