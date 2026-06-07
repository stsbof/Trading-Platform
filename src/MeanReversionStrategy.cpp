#include "MeanReversionStrategy.h"

MeanReversionStrategy::MeanReversionStrategy(std::size_t window, double k)
    : prices_(window), k_(k) {}

Signal MeanReversionStrategy::on_market_data(const MarketData& data) {
    if (data.best_bid <= 0.0 || data.best_ask <= 0.0) return Signal::HOLD;

    double mid = (data.best_bid + data.best_ask) / 2.0;
    prices_.push(mid);

    if (!prices_.full()) return Signal::HOLD;

    double mean   = prices_.mean();
    double stddev = prices_.stddev();
    if (stddev < 1e-9) return Signal::HOLD;

    double lower = mean - k_ * stddev;
    double upper = mean + k_ * stddev;

    if (mid < lower) return Signal::BUY;
    if (mid > upper) return Signal::SELL;
    return Signal::HOLD;
}
