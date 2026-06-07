#pragma once
#include <cstdint>
#include <string>

enum class Side { BID, ASK };
enum class OrderType { MARKET, LIMIT };
enum class Signal { BUY, SELL, HOLD };

struct OrderAdd {
    int64_t     order_id;
    Side        side;
    double      price;
    int64_t     quantity;
    int64_t     timestamp;
};

struct OrderRemove {
    int64_t order_id;
};

struct MarketData {
    int64_t timestamp;
    double  best_bid;
    double  best_ask;
    double  last_price;
    int64_t volume;
};

struct Order {
    int64_t   order_id;
    OrderType type;
    Side      side;
    double    price;   // ignored for MARKET orders
    int64_t   quantity;
    std::string symbol;
};

struct Trade {
    int64_t order_id;
    Side    side;
    double  price;
    int64_t quantity;
    int64_t timestamp;
    std::string symbol;
};
