#include <gtest/gtest.h>
#include "Portfolio.h"

static Trade make_trade(Side side, double price, int64_t qty,
                         const std::string& sym = "BTC") {
    static int64_t id = 1;
    return Trade{id++, side, price, qty, 1700000000, sym};
}

TEST(Portfolio, InitialState) {
    Portfolio p(100000.0, 50);
    EXPECT_DOUBLE_EQ(p.cash(), 100000.0);
    EXPECT_DOUBLE_EQ(p.total_pnl(), 0.0);
    EXPECT_EQ(p.position("BTC"), 0);
}

TEST(Portfolio, BuyUpdatesCash) {
    Portfolio p(100000.0, 50);
    p.on_trade(make_trade(Side::BID, 50000.0, 1));
    EXPECT_DOUBLE_EQ(p.cash(), 50000.0);
    EXPECT_EQ(p.position("BTC"), 1);
}

TEST(Portfolio, BuySellRealisedPnL) {
    Portfolio p(200000.0, 50);
    p.on_trade(make_trade(Side::BID,  50000.0, 2));
    p.on_trade(make_trade(Side::ASK,  51000.0, 2));
    // P&L = (51000 - 50000) * 2 = 2000
    EXPECT_DOUBLE_EQ(p.total_pnl(), 2000.0);
    EXPECT_EQ(p.position("BTC"), 0);
}

TEST(Portfolio, RiskCheckRejectsLargePosition) {
    Portfolio p(1000000.0, 10);
    Order o{1, OrderType::MARKET, Side::BID, 0.0, 15, "BTC"};
    EXPECT_FALSE(p.check_risk(o));
}

TEST(Portfolio, RiskCheckRejectsShortSell) {
    Portfolio p(1000000.0, 50);
    Order o{1, OrderType::MARKET, Side::ASK, 0.0, 1, "BTC"};
    EXPECT_FALSE(p.check_risk(o));  // No inventory → reject
}

TEST(Portfolio, RiskCheckAllowsSmallPosition) {
    Portfolio p(1000000.0, 50);
    Order o{1, OrderType::MARKET, Side::BID, 0.0, 5, "BTC"};
    EXPECT_TRUE(p.check_risk(o));
}

TEST(Portfolio, PnlSeriesGrowsWithTrades) {
    Portfolio p(200000.0, 50);
    p.on_trade(make_trade(Side::BID, 50000.0, 1));
    p.on_trade(make_trade(Side::ASK, 51000.0, 1));
    EXPECT_EQ(p.pnl_series().size(), 2u);
}
