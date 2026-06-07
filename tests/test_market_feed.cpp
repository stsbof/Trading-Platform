#include <gtest/gtest.h>
#include "MarketDataFeed.h"
#include <fstream>
#include <variant>

static std::string write_tmp(const std::string& content) {
    std::string path = "/tmp/feed_test_" + std::to_string(std::rand()) + ".csv";
    std::ofstream f(path);
    f << content;
    return path;
}

TEST(MarketDataFeed, ParseAdd) {
    auto path = write_tmp(
        "timestamp,event_type,order_id,side,price,quantity\n"
        "1700000001,ADD,1001,BID,42500.00,5\n");
    MarketDataFeed feed(path);
    auto ev = feed.next_event();
    ASSERT_TRUE(ev.has_value());
    ASSERT_TRUE(std::holds_alternative<OrderAdd>(*ev));
    auto& add = std::get<OrderAdd>(*ev);
    EXPECT_EQ(add.order_id, 1001);
    EXPECT_EQ(add.side, Side::BID);
    EXPECT_DOUBLE_EQ(add.price, 42500.00);
    EXPECT_EQ(add.quantity, 5);
}

TEST(MarketDataFeed, ParseRemove) {
    auto path = write_tmp(
        "timestamp,event_type,order_id,side,price,quantity\n"
        "1700000004,REMOVE,1001,,,\n");
    MarketDataFeed feed(path);
    auto ev = feed.next_event();
    ASSERT_TRUE(ev.has_value());
    ASSERT_TRUE(std::holds_alternative<OrderRemove>(*ev));
    EXPECT_EQ(std::get<OrderRemove>(*ev).order_id, 1001);
}

TEST(MarketDataFeed, ExhaustsCorrectly) {
    auto path = write_tmp(
        "timestamp,event_type,order_id,side,price,quantity\n"
        "1700000001,ADD,1001,ASK,100.0,3\n");
    MarketDataFeed feed(path);
    EXPECT_TRUE(feed.next_event().has_value());
    EXPECT_FALSE(feed.next_event().has_value());
}

TEST(MarketDataFeed, MissingFileThrows) {
    EXPECT_THROW(MarketDataFeed("/nonexistent/path.csv"), std::runtime_error);
}

TEST(MarketDataFeed, InvalidSideThrows) {
    auto path = write_tmp(
        "timestamp,event_type,order_id,side,price,quantity\n"
        "1700000001,ADD,1001,XYZ,100.0,3\n");
    MarketDataFeed feed(path);
    EXPECT_THROW(feed.next_event(), std::runtime_error);
}

TEST(MarketDataFeed, NegativePriceThrows) {
    auto path = write_tmp(
        "timestamp,event_type,order_id,side,price,quantity\n"
        "1700000001,ADD,1001,BID,-5.0,3\n");
    MarketDataFeed feed(path);
    EXPECT_THROW(feed.next_event(), std::runtime_error);
}
