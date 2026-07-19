#include "event_bus.h"

#include <algorithm>

namespace events {

EventBus::SubscriptionId EventBus::subscribe(Handler handler) {
    const SubscriptionId id = next_id_++;
    subscriptions_.push_back({id, std::move(handler)});
    return id;
}

void EventBus::unsubscribe(SubscriptionId id) {
    subscriptions_.erase(
        std::remove_if(subscriptions_.begin(), subscriptions_.end(),
                       [id](const Subscription& subscription) { return subscription.id == id; }),
        subscriptions_.end());
}

void EventBus::publish(const GameEvent& event) {
    for (const Subscription& subscription : subscriptions_) {
        subscription.handler(event);
    }
}

}  // namespace events
