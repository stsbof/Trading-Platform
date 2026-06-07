#include "OrderBook.h"
#include <iomanip>
#include <stdexcept>
#include <algorithm>

void OrderBook::on_add(const OrderAdd& event) {
    if (event.quantity <= 0)
        throw std::runtime_error("OrderBook::on_add: non-positive quantity");

    order_index_[event.order_id] = {event.side, event.price};
    last_timestamp_ = event.timestamp;

    if (event.side == Side::BID)
        bids_[event.price].push_back(event);
    else
        asks_[event.price].push_back(event);
}

void OrderBook::on_remove(const OrderRemove& event) {
    auto it = order_index_.find(event.order_id);
    if (it == order_index_.end())
        return; // silent: order may have been consumed by matching

    auto [side, price] = it->second;
    order_index_.erase(it);

    if (side == Side::BID) {
        auto bit = bids_.find(price);
        if (bit != bids_.end()) {
            auto& dq = bit->second;
            auto oit = std::find_if(dq.begin(), dq.end(),
                [&](const OrderAdd& o){ return o.order_id == event.order_id; });
            if (oit != dq.end()) dq.erase(oit);
            if (dq.empty()) bids_.erase(bit);
        }
    } else {
        auto ait = asks_.find(price);
        if (ait != asks_.end()) {
            auto& dq = ait->second;
            auto oit = std::find_if(dq.begin(), dq.end(),
                [&](const OrderAdd& o){ return o.order_id == event.order_id; });
            if (oit != dq.end()) dq.erase(oit);
            if (dq.empty()) asks_.erase(ait);
        }
    }
}

// All-or-nothing, price-time priority — aggregates across multiple price levels
std::optional<Trade> OrderBook::match(const Order& order) {
    if (order.side == Side::BID) {
        // BUY: consume from the ask side (asks sorted ascending: cheapest first)
        if (asks_.empty()) return std::nullopt;

        // Pass 1: check total available liquidity across compatible levels
        int64_t total_available = 0;
        for (auto& [price, queue] : asks_) {
            bool price_ok = (order.type == OrderType::MARKET) || (order.price >= price);
            if (!price_ok) break; // asks are ascending; further levels are more expensive
            for (const auto& o : queue) total_available += o.quantity;
            if (total_available >= order.quantity) break;
        }
        if (total_available < order.quantity) return std::nullopt;

        // Pass 2: consume across levels, computing volume-weighted avg price
        double weighted_sum = 0.0;
        int64_t remaining   = order.quantity;

        for (auto it = asks_.begin(); it != asks_.end() && remaining > 0; ) {
            double level_price = it->first;
            bool price_ok = (order.type == OrderType::MARKET) || (order.price >= level_price);
            if (!price_ok) break;

            auto& queue = it->second;
            while (remaining > 0 && !queue.empty()) {
                auto& front = queue.front();
                int64_t take   = std::min(remaining, front.quantity);
                weighted_sum  += static_cast<double>(take) * level_price;
                front.quantity -= take;
                remaining      -= take;
                if (front.quantity == 0) {
                    order_index_.erase(front.order_id);
                    queue.pop_front();
                }
            }
            it = queue.empty() ? asks_.erase(it) : std::next(it);
        }

        double avg_price = weighted_sum / static_cast<double>(order.quantity);
        last_price_  = avg_price;
        last_volume_ = order.quantity;

        return Trade{order.order_id, order.side, avg_price,
                     order.quantity, last_timestamp_, order.symbol};
    } else {
        // SELL: consume from the bid side (bids sorted descending: highest first)
        if (bids_.empty()) return std::nullopt;

        // Pass 1: check total available liquidity across compatible levels
        int64_t total_available = 0;
        for (auto& [price, queue] : bids_) {
            bool price_ok = (order.type == OrderType::MARKET) || (order.price <= price);
            if (!price_ok) break; // bids are descending; further levels are lower
            for (const auto& o : queue) total_available += o.quantity;
            if (total_available >= order.quantity) break;
        }
        if (total_available < order.quantity) return std::nullopt;

        // Pass 2: consume, volume-weighted avg price
        double weighted_sum = 0.0;
        int64_t remaining   = order.quantity;

        for (auto it = bids_.begin(); it != bids_.end() && remaining > 0; ) {
            double level_price = it->first;
            bool price_ok = (order.type == OrderType::MARKET) || (order.price <= level_price);
            if (!price_ok) break;

            auto& queue = it->second;
            while (remaining > 0 && !queue.empty()) {
                auto& front = queue.front();
                int64_t take   = std::min(remaining, front.quantity);
                weighted_sum  += static_cast<double>(take) * level_price;
                front.quantity -= take;
                remaining      -= take;
                if (front.quantity == 0) {
                    order_index_.erase(front.order_id);
                    queue.pop_front();
                }
            }
            it = queue.empty() ? bids_.erase(it) : std::next(it);
        }

        double avg_price = weighted_sum / static_cast<double>(order.quantity);
        last_price_  = avg_price;
        last_volume_ = order.quantity;

        return Trade{order.order_id, order.side, avg_price,
                     order.quantity, last_timestamp_, order.symbol};
    }
}

MarketData OrderBook::snapshot() const {
    double bb = best_bid();
    double ba = best_ask();
    double lp = (last_price_ > 0) ? last_price_ : (bb > 0 ? bb : ba);
    return MarketData{last_timestamp_, bb, ba, lp, last_volume_};
}

double OrderBook::best_bid() const {
    if (bids_.empty()) return 0.0;
    return bids_.begin()->first;
}

double OrderBook::best_ask() const {
    if (asks_.empty()) return 0.0;
    return asks_.begin()->first;
}

double OrderBook::spread() const {
    if (bids_.empty() || asks_.empty()) return 0.0;
    return best_ask() - best_bid();
}

void OrderBook::display(int levels) const {
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "  --- ASK ---\n";
    int shown = 0;
    for (auto it = asks_.rbegin(); it != asks_.rend() && shown < levels; ++it, ++shown) {
        int64_t total_qty = 0;
        for (const auto& o : it->second) total_qty += o.quantity;
        std::cout << "  " << std::setw(10) << it->first
                  << "  qty: " << total_qty << "\n";
    }
    std::cout << "  -----------\n";
    shown = 0;
    for (auto& [price, queue] : bids_) {
        if (shown++ >= levels) break;
        int64_t total_qty = 0;
        for (const auto& o : queue) total_qty += o.quantity;
        std::cout << "  " << std::setw(10) << price
                  << "  qty: " << total_qty << "\n";
    }
    std::cout << "  --- BID ---\n";
}
