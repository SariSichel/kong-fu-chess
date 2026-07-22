#pragma once

#include <cstdint>
#include <functional>
#include <utility>
#include <variant>
#include <vector>

#include "game_event.h"

namespace events {

class EventBus {
public:
    using Handler = std::function<void(const GameEvent&)>;
    using SubscriptionId = std::uint64_t;

    SubscriptionId subscribe(Handler handler);
    SubscriptionId subscribeAny(Handler handler) { return subscribe(std::move(handler)); }

    template <typename T, typename Fn>
    SubscriptionId subscribe(Fn&& handler) {
        return subscribe([h = std::forward<Fn>(handler)](const GameEvent& event) {
            if (const auto* typed = std::get_if<T>(&event)) {
                h(*typed);
            }
        });
    }

    void unsubscribe(SubscriptionId id);
    void publish(const GameEvent& event);

private:
    struct Subscription {
        SubscriptionId id = 0;
        Handler handler;
    };

    std::vector<Subscription> subscriptions_;
    SubscriptionId next_id_ = 1;
};

}  // namespace events
