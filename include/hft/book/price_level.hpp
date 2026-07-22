#pragma once

#include "hft/core/order.hpp"
#include <deque>

namespace hft {

struct PriceLevel {
    Price    price;
    std::deque<Order*> orders;
    Quantity total_volume = 0;

    void push_back(Order* o) {
        orders.push_back(o);
        total_volume += o->remaining();
    }

    // Iter 1: O(n) linear scan. Iter 3: intrusive linked list (O(1)).
    bool remove(OrderId order_id) {
        for (auto it = orders.begin(); it != orders.end(); ++it) {
            if ((*it)->id == order_id) {
                total_volume -= (*it)->remaining();
                orders.erase(it);
                return true;
            }
        }
        return false;
    }

    Order* front() const {
        return orders.empty() ? nullptr : orders.front();
    }

    bool empty() const {
        return orders.empty();
    }

    void pop_front() {
        if (!orders.empty()) {
            total_volume -= orders.front()->remaining();
            orders.pop_front();
        }
    }
};

} // namespace hft
