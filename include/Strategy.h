#pragma once
#include "Types.h"
#include <string>

class Strategy {
public:
    virtual ~Strategy() = default;
    virtual Signal on_market_data(const MarketData& data) = 0;
    virtual std::string name() const = 0;
};
