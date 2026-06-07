cmake_minimum_required(VERSION 3.15)
project(TradingPlatform CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Source files shared by main executable and tests ──────────────────
set(LIB_SOURCES
    src/MarketDataFeed.cpp
    src/OrderBook.cpp
    src/MeanReversionStrategy.cpp
    src/TrendFollowingStrategy.cpp
    src/Portfolio.cpp
    src/Reporting.cpp
)

# ── Main executable ───────────────────────────────────────────────────
add_executable(trading_sim
    src/main.cpp
    ${LIB_SOURCES}
)
target_include_directories(trading_sim PRIVATE include)

# ── Google Test (fetched automatically if not installed) ──────────────
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_subdirectory(tests)
