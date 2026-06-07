#include "Portfolio.h"
#include <iostream>
#include <stdexcept>

Portfolio::Portfolio(double initial_cash, int64_t max_position)
    : initial_cash_(initial_cash), cash_(initial_cash), max_position_(max_position) {}

int64_t Portfolio::position(const std::string& symbol) const {
    auto it = positions_.find(symbol);
    return (it != positions_.end()) ? it->second : 0;
}

bool Portfolio::check_risk(const Order& order) const {
    int64_t current   = position(order.symbol);
    int64_t delta     = (order.side == Side::BID) ? order.quantity : -order.quantity;
    int64_t projected = current + delta;

    // No short selling: reject SELL if we don't have enough inventory
    if (order.side == Side::ASK && current < static_cast<int64_t>(order.quantity)) {
        std::cout << "[RISK] Order rejected: cannot SELL " << order.quantity
                  << " of '" << order.symbol << "' (position=" << current << ")\n";
        return false;
    }

    if (std::abs(projected) > max_position_) {
        std::cout << "[RISK] Order rejected: projected position " << projected
                  << " exceeds max " << max_position_
                  << " for symbol '" << order.symbol << "'\n";
        return false;
    }
    return true;
}

bool Portfolio::on_trade(const Trade& trade) {
    // Internal safety check: makes this module autonomous even if the caller
    // forgot to call check_risk() before matching.
    Order synthetic{trade.order_id, OrderType::MARKET, trade.side,
                    trade.price, trade.quantity, trade.symbol};
    if (!check_risk(synthetic)) return false;

    const std::string& sym = trade.symbol;
    int64_t& pos  = positions_[sym];
    double&  cost = avg_cost_[sym];

    double pnl_this_trade = 0.0;

    if (trade.side == Side::BID) {
        // Buying: increase position
        double total_cost = cost * pos + trade.price * trade.quantity;
        pos  += trade.quantity;
        cost  = (pos != 0) ? (total_cost / pos) : 0.0;
        cash_ -= trade.price * trade.quantity;
    } else {
        // Selling: realise P&L
        pnl_this_trade = (trade.price - cost) * trade.quantity;
        realized_pnl_ += pnl_this_trade;
        pos  -= trade.quantity;
        cash_ += trade.price * trade.quantity;
        if (pos == 0) cost = 0.0;
    }

    trades_.push_back(trade);
    pnl_series_.push_back(pnl_this_trade);
    return true;
}
