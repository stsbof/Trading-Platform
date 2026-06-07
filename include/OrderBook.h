#pragma once
#include "Types.h"
#include "Events.h"
#include <map>
#include <deque>
#include <unordered_map>
#include <optional>
#include <iostream>

class OrderBook {
public:
    // Feed alimentation
    void on_add(const OrderAdd& event);
    void on_remove(const OrderRemove& event);

    // Match a strategy order; all-or-nothing semantics
    std::optional<Trade> match(const Order& order);

    // Generate snapshot after each book update
    MarketData snapshot() const;

    double best_bid() const;
    double best_ask() const;
    double spread()   const;

    void display(int levels) const;

private:
    struct PriceLevel {
        double  price;
        std::deque<OrderAdd> orders;  // FIFO for time-priority
    };

    // bids: highest price first  → greater<double>
    // asks: lowest  price first  → default less<double>
    std::map<double, std::deque<OrderAdd>, std::greater<double>> bids_;
    std::map<double, std::deque<OrderAdd>>                       asks_;

    // Fast lookup: order_id → side + price for O(1) removal
    std::unordered_map<int64_t, std::pair<Side, double>> order_index_;

    double  last_price_{0.0};
    int64_t last_timestamp_{0};
    int64_t last_volume_{0};

};
