#pragma once
#include "Strategy.h"
#include "TimeSeries.h"

// Bollinger Bands mean-reversion strategy.
// BUY  when price < mean - k * stddev  (below lower band)
// SELL when price > mean + k * stddev  (above upper band)
// HOLD otherwise
class MeanReversionStrategy : public Strategy {
public:
    explicit MeanReversionStrategy(std::size_t window = 20, double k = 2.0);

    Signal      on_market_data(const MarketData& data) override;
    std::string name() const override { return "MeanReversion-BollingerBands"; }

private:
    TimeSeries<double> prices_;
    double             k_;
};
