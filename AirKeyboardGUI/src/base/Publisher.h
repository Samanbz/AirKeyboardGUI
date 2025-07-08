#pragma once
#include <memory>
#include <mutex>
#include <vector>

#include "Subscriber.h"

template <typename MessageType>
class Publisher {
    typedef Subscriber<MessageType> SubscriberType;

protected:
    std::vector<SubscriberType*> subscribers;
    std::mutex subscribersLock;

public:
    Publisher() : subscribers() {}

    void subscribe(SubscriberType* sub) {
        if (sub) {
            std::lock_guard<std::mutex> lock(subscribersLock);
            subscribers.push_back(sub);
        }
    }

    void unsubscribe(SubscriberType* sub) {
        if (sub) {
            std::lock_guard<std::mutex> lock(subscribersLock);
            auto it = std::remove(subscribers.begin(), subscribers.end(), sub);
            if (it != subscribers.end()) {
                subscribers.erase(it, subscribers.end());
            }
        }
    }

    void publish(std::shared_ptr<MessageType> message) {
        std::lock_guard<std::mutex> lock(subscribersLock);
        for (auto& sub : subscribers) {
            sub->enqueue(message);
        }
    }

    virtual void shutdown() {
        std::lock_guard<std::mutex> lock(subscribersLock);
        subscribers.clear();
    }

    virtual ~Publisher() {
        shutdown();
    }
};