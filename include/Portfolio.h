#pragma once
#include "Types.h"
#include <unordered_map>
#include <string>
#include <vector>

class Portfolio {
public:
    explicit Portfolio(double initial_cash, int64_t max_position = 100);

    // Record an executed trade and update positions/P&L.
    // Also performs an internal risk check (no short-sell, position cap).
    // Returns true on success, false if the trade violates a risk rule.
    // NOTE: call check_risk(order) BEFORE match() to avoid wasting a book fill.
    bool on_trade(const Trade& trade);

    // Pre-trade check only (does not modify state)
    bool check_risk(const Order& order) const;

    double  cash()          const { return cash_; }
    double  initial_cash()  const { return initial_cash_; }
    double  total_pnl()     const { return realized_pnl_; }
    int64_t position(const std::string& symbol) const;

    const std::vector<double>& pnl_series() const { return pnl_series_; }
    const std::vector<Trade>&  trades()     const { return trades_; }

private:
    double  initial_cash_;
    double  cash_;
    double  realized_pnl_{0.0};
    int64_t max_position_;

    std::unordered_map<std::string, int64_t> positions_;  // symbol → net position
    std::unordered_map<std::string, double>  avg_cost_;   // symbol → average cost

    std::vector<Trade>  trades_;
    std::vector<double> pnl_series_;  // P&L after each trade (for Sharpe)
};
