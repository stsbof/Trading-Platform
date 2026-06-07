#include "MarketDataFeed.h"
#include "OrderBook.h"
#include "MeanReversionStrategy.h"
#include "TrendFollowingStrategy.h"
#include "Portfolio.h"
#include "Reporting.h"

#include <iostream>
#include <memory>
#include <string>
#include <variant>
#include <stdexcept>
#include <iomanip>

static int64_t g_order_id = 10000;

int main(int argc, char* argv[]) {
    // ── Configuration ──────────────────────────────────────────────────
    std::string csv_path      = "../data/market_data.csv";
    std::string strategy_name = "mean_reversion";
    double initial_cash       = 1'000'000.0;
    int64_t max_position      = 50;
    std::string symbol        = "BTC";
    int64_t order_qty         = 1;

    if (argc >= 2) csv_path      = argv[1];
    if (argc >= 3) strategy_name = argv[2];

    // ── Build components ───────────────────────────────────────────────
    std::unique_ptr<Strategy> strategy;
    if (strategy_name == "trend_following")
        strategy = std::make_unique<TrendFollowingStrategy>(10, 30);
    else
        strategy = std::make_unique<MeanReversionStrategy>(15, 1.0);

    OrderBook  book;
    Portfolio  portfolio(initial_cash, max_position);
    Reporting  reporter(0.0);

    int events_processed = 0;
    int trades_executed  = 0;

    // ── Main simulation loop ───────────────────────────────────────────
    // Sequential, explicit pipeline — no re-entrancy:
    //   feed → book update → snapshot → strategy → risk check → match → portfolio
    try {
        MarketDataFeed feed(csv_path);

        while (auto ev = feed.next_event()) {
            ++events_processed;

            // Step 1: update the order book
            std::visit([&](auto&& e) {
                using T = std::decay_t<decltype(e)>;
                if constexpr (std::is_same_v<T, OrderAdd>)
                    book.on_add(e);
                else if constexpr (std::is_same_v<T, OrderRemove>)
                    book.on_remove(e);
            }, *ev);

            // Step 2: generate MarketData snapshot
            MarketData md = book.snapshot();
            if (md.best_bid <= 0.0 || md.best_ask <= 0.0) continue;

            // Step 3: ask strategy for a signal
            Signal sig = strategy->on_market_data(md);
            if (sig == Signal::HOLD) continue;

            // Step 4: build the order
            Side order_side = (sig == Signal::BUY) ? Side::BID : Side::ASK;
            Order order{
                ++g_order_id,
                OrderType::MARKET,
                order_side,
                0.0,
                order_qty,
                symbol
            };

            // Step 5: pre-trade risk check
            if (!portfolio.check_risk(order)) continue;

            // Step 6: attempt matching
            auto trade_opt = book.match(order);
            if (!trade_opt) continue;

            // Step 7: record in portfolio
            if (!portfolio.on_trade(*trade_opt)) continue;
            ++trades_executed;

            std::cout << std::fixed << std::setprecision(2);
            std::cout << "[TRADE #" << std::setw(3) << trades_executed << "] "
                      << (order_side == Side::BID ? "BUY " : "SELL")
                      << "  qty=" << trade_opt->quantity
                      << "  @" << trade_opt->price
                      << "  pos=" << portfolio.position(symbol)
                      << "  pnl=" << portfolio.total_pnl()
                      << "\n";
        }

        std::cout << "\nSimulation complete."
                  << " Events: " << events_processed
                  << " | Trades: " << trades_executed << "\n";

    } catch (const std::exception& ex) {
        std::cerr << "[ERROR] " << ex.what() << "\n";
        return 1;
    }

    // ── Reporting ──────────────────────────────────────────────────────
    reporter.generate(portfolio, strategy->name());

    return 0;
}
