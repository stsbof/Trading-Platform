add_executable(trading_tests
    test_order_book.cpp
    test_market_feed.cpp
    test_portfolio.cpp
)

target_include_directories(trading_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_sources(trading_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/MarketDataFeed.cpp
    ${CMAKE_SOURCE_DIR}/src/OrderBook.cpp
    ${CMAKE_SOURCE_DIR}/src/MeanReversionStrategy.cpp
    ${CMAKE_SOURCE_DIR}/src/TrendFollowingStrategy.cpp
    ${CMAKE_SOURCE_DIR}/src/Portfolio.cpp
    ${CMAKE_SOURCE_DIR}/src/Reporting.cpp
)

target_link_libraries(trading_tests PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(trading_tests)
