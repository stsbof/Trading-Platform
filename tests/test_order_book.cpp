#include <gtest/gtest.h>
#include "OrderBook.h"

static OrderAdd make_add(int64_t id, Side side, double price, int64_t qty,
                          int64_t ts = 1700000000) {
    return OrderAdd{id, side, price, qty, ts};
}

TEST(OrderBook, BestBidAskEmpty) {
    OrderBook book;
    EXPECT_DOUBLE_EQ(book.best_bid(), 0.0);
    EXPECT_DOUBLE_EQ(book.best_ask(), 0.0);
}

TEST(OrderBook, AddBidAsk) {
    OrderBook book;
    book.on_add(make_add(1, Side::BID, 100.0, 5));
    book.on_add(make_add(2, Side::ASK, 101.0, 3));
    EXPECT_DOUBLE_EQ(book.best_bid(), 100.0);
    EXPECT_DOUBLE_EQ(book.best_ask(), 101.0);
    EXPECT_DOUBLE_EQ(book.spread(),    1.0);
}

TEST(OrderBook, RemoveOrder) {
    OrderBook book;
    book.on_add(make_add(1, Side::BID, 100.0, 5));
    book.on_add(make_add(2, Side::BID, 99.0,  3));
    book.on_remove(OrderRemove{1});
    EXPECT_DOUBLE_EQ(book.best_bid(), 99.0);
}

TEST(OrderBook, MarketOrderMatchBuy) {
    OrderBook book;
    book.on_add(make_add(1, Side::ASK, 101.0, 10));

    Order o{9001, OrderType::MARKET, Side::BID, 0.0, 5, "BTC"};
    auto t = book.match(o);
    ASSERT_TRUE(t.has_value());
    EXPECT_DOUBLE_EQ(t->price,    101.0);
    EXPECT_EQ(t->quantity, 5);
    EXPECT_EQ(t->side,     Side::BID);
}

TEST(OrderBook, MarketOrderMatchSell) {
    OrderBook book;
    book.on_add(make_add(1, Side::BID, 100.0, 10));

    Order o{9002, OrderType::MARKET, Side::ASK, 0.0, 4, "BTC"};
    auto t = book.match(o);
    ASSERT_TRUE(t.has_value());
    EXPECT_DOUBLE_EQ(t->price,    100.0);
    EXPECT_EQ(t->quantity, 4);
}

TEST(OrderBook, AllOrNothingRejectPartial) {
    OrderBook book;
    // Only 3 units available total, ask for 10 → must reject
    book.on_add(make_add(1, Side::ASK, 101.0, 3));

    Order o{9003, OrderType::MARKET, Side::BID, 0.0, 10, "BTC"};
    auto t = book.match(o);
    EXPECT_FALSE(t.has_value());
}

TEST(OrderBook, MultiLevelMatchBuy) {
    OrderBook book;
    // 5 @100, 5 @101 → BUY 10 should aggregate both levels
    book.on_add(make_add(1, Side::ASK, 100.0, 5));
    book.on_add(make_add(2, Side::ASK, 101.0, 5));

    Order o{9010, OrderType::MARKET, Side::BID, 0.0, 10, "BTC"};
    auto t = book.match(o);
    ASSERT_TRUE(t.has_value());
    EXPECT_EQ(t->quantity, 10);
    // Avg price = (5*100 + 5*101) / 10 = 100.5
    EXPECT_DOUBLE_EQ(t->price, 100.5);
}

TEST(OrderBook, MultiLevelMatchSell) {
    OrderBook book;
    // bid 5 @100, bid 5 @99 → SELL 10 should aggregate both levels
    book.on_add(make_add(1, Side::BID, 100.0, 5));
    book.on_add(make_add(2, Side::BID, 99.0,  5));

    Order o{9011, OrderType::MARKET, Side::ASK, 0.0, 10, "BTC"};
    auto t = book.match(o);
    ASSERT_TRUE(t.has_value());
    EXPECT_EQ(t->quantity, 10);
    // Avg price = (5*100 + 5*99) / 10 = 99.5
    EXPECT_DOUBLE_EQ(t->price, 99.5);
}

TEST(OrderBook, LimitOrderPriceTooLow) {
    OrderBook book;
    book.on_add(make_add(1, Side::ASK, 105.0, 5));

    // Limit buy at 100 — cannot match ask at 105
    Order o{9004, OrderType::LIMIT, Side::BID, 100.0, 2, "BTC"};
    auto t = book.match(o);
    EXPECT_FALSE(t.has_value());
}

TEST(OrderBook, LimitOrderMatchable) {
    OrderBook book;
    book.on_add(make_add(1, Side::ASK, 105.0, 5));

    // Limit buy at 106 — price >= ask → should match
    Order o{9005, OrderType::LIMIT, Side::BID, 106.0, 3, "BTC"};
    auto t = book.match(o);
    ASSERT_TRUE(t.has_value());
    EXPECT_DOUBLE_EQ(t->price, 105.0);
}

TEST(OrderBook, SnapshotAfterAdd) {
    OrderBook book;
    book.on_add(make_add(1, Side::BID, 100.0, 5, 1700000001));
    book.on_add(make_add(2, Side::ASK, 102.0, 3, 1700000002));

    auto md = book.snapshot();
    EXPECT_DOUBLE_EQ(md.best_bid, 100.0);
    EXPECT_DOUBLE_EQ(md.best_ask, 102.0);
}
