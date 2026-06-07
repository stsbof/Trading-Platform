#include "TrendFollowingStrategy.h"

TrendFollowingStrategy::TrendFollowingStrategy(int fast_period, int slow_period)
    : fast_period_(fast_period)
    , slow_period_(slow_period)
    , alpha_fast_(2.0 / (fast_period + 1))
    , alpha_slow_(2.0 / (slow_period + 1)) {}

double TrendFollowingStrategy::update_ema(double prev, double price, double alpha) const {
    return alpha * price + (1.0 - alpha) * prev;
}

Signal TrendFollowingStrategy::on_market_data(const MarketData& data) {
    if (data.best_bid <= 0.0 || data.best_ask <= 0.0) return Signal::HOLD;

    ++ticks_;
    double price = (data.best_bid + data.best_ask) / 2.0;

    if (ticks_ == 1) {
        fast_ema_ = price;
        slow_ema_ = price;
        prev_fast_above_ = false;
        return Signal::HOLD;
    }

    fast_ema_ = update_ema(fast_ema_, price, alpha_fast_);
    slow_ema_ = update_ema(slow_ema_, price, alpha_slow_);

    // Need enough data for the slow EMA to be meaningful
    if (ticks_ < slow_period_) return Signal::HOLD;

    bool fast_above = fast_ema_ > slow_ema_;

    Signal sig = Signal::HOLD;
    if (fast_above && !prev_fast_above_)
        sig = Signal::BUY;    // crossover up
    else if (!fast_above && prev_fast_above_)
        sig = Signal::SELL;   // crossover down

    prev_fast_above_ = fast_above;
    return sig;
}
