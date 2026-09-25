#pragma once

#include "hft/core/order.hpp"
#include <deque>

namespace hft {

// 单个价格档位 — 同一价位的订单按 FIFO（时间优先）排队
struct PriceLevel {
    Price price;               // 该档位价格
    std::deque<Order*> orders; // 排队订单（front = 最早挂入）
    Quantity total_volume = 0; // 该档位总挂单量（避免每次遍历累加）

    // 挂入订单（追加到队列尾部）
    void push_back(Order* o) {
        orders.push_back(o);
        total_volume += o->remaining();
    }

    // 删除指定订单（Iter 1: O(n) 线性查找；Iter 3: 换侵入式链表 O(1)）
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

    // 获取队首订单（最早挂入）
    Order* front() const { return orders.empty() ? nullptr : orders.front(); }

    // 队列是否为空
    bool empty() const { return orders.empty(); }

    // 弹出队首订单
    void pop_front() {
        if (!orders.empty()) {
            total_volume -= orders.front()->remaining();
            orders.pop_front();
        }
    }
};

} // namespace hft
