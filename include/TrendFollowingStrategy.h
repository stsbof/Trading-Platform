#pragma once
#include "Strategy.h"

// EMA crossover trend-following strategy.
// BUY  when fast EMA crosses above slow EMA
// SELL when fast EMA crosses below slow EMA
// HOLD otherwise
class TrendFollowingStrategy : public Strategy {
public:
    explicit TrendFollowingStrategy(int fast_period = 10, int slow_period = 30);

    Signal      on_market_data(const MarketData& data) override;
    std::string name() const override { return "TrendFollowing-EMACrossover"; }

private:
    double update_ema(double prev_ema, double price, double alpha) const;

    int    fast_period_;
    int    slow_period_;
    double alpha_fast_;
    double alpha_slow_;

    double fast_ema_{0.0};
    double slow_ema_{0.0};
    int    ticks_{0};

    bool   prev_fast_above_{false};
};
