#include "Reporting.h"
#include <iostream>
#include <iomanip>
#include <numeric>
#include <cmath>
#include <algorithm>

Reporting::Reporting(double risk_free_rate)
    : risk_free_rate_(risk_free_rate) {}

void Reporting::generate(const Portfolio& portfolio,
                         const std::string& strategy_name) const {
    const auto& pnl  = portfolio.pnl_series();
    const auto& trd  = portfolio.trades();

    double total_pnl   = portfolio.total_pnl();
    int    nb_trades   = static_cast<int>(trd.size());
    double sharpe      = compute_sharpe(pnl, portfolio.initial_cash());
    double max_dd      = compute_max_drawdown(pnl);
    double win_rate    = compute_win_rate(trd, pnl);

    const int W = 30;
    std::cout << "\n";
    std::cout << std::string(55, '=') << "\n";
    std::cout << "  POST-TRADE REPORT  |  Strategy: " << strategy_name << "\n";
    std::cout << std::string(55, '=') << "\n";
    std::cout << std::fixed << std::setprecision(4);
    std::cout << std::left  << std::setw(W) << "  P&L Total (realised)"
              << std::right << std::setw(12) << total_pnl << "\n";
    std::cout << std::left  << std::setw(W) << "  Number of Trades"
              << std::right << std::setw(12) << nb_trades << "\n";
    std::cout << std::left  << std::setw(W) << "  Sharpe Ratio (annualised)"
              << std::right << std::setw(12) << sharpe << "\n";
    std::cout << std::left  << std::setw(W) << "  Max Drawdown"
              << std::right << std::setw(12) << max_dd << "\n";
    std::cout << std::left  << std::setw(W) << "  Win Rate"
              << std::right << std::setw(11) << win_rate * 100.0 << "%\n";
    std::cout << std::string(55, '=') << "\n\n";
}

double Reporting::compute_sharpe(const std::vector<double>& pnl,
                                  double initial_equity) const {
    // Build a series of percentage returns, one per CLOSING event (non-zero P&L).
    //
    // r_t = pnl_t / equity_{t-1}
    //
    // where equity_{t-1} = initial_equity + cumulative realised P&L before trade t.
    // BUY trades have pnl=0 and are skipped — they don't close a position.
    //
    // Formula (subject): S = (r̄ - rf) / σr × √252
    // √252 annualises assuming each closing trade ≈ one trading day.

    std::vector<double> returns;
    double equity = initial_equity;

    for (double p : pnl) {
        if (std::abs(p) < 1e-9) {
            equity += p;   // equity unchanged for zero-pnl trades (BUY)
            continue;
        }
        double prev_equity = equity;
        equity += p;
        if (prev_equity > 1e-9)                    // guard against zero division
            returns.push_back(p / prev_equity);
    }

    if (returns.size() < 2) return 0.0;

    double mean = std::accumulate(returns.begin(), returns.end(), 0.0)
                  / static_cast<double>(returns.size());
    double var  = 0.0;
    for (double r : returns) var += (r - mean) * (r - mean);
    double stddev = std::sqrt(var / static_cast<double>(returns.size()));

    if (stddev < 1e-12) return 0.0;
    return (mean - risk_free_rate_) / stddev * std::sqrt(252.0);
}

double Reporting::compute_max_drawdown(const std::vector<double>& pnl) const {
    if (pnl.empty()) return 0.0;

    double peak    = 0.0;
    double cum     = 0.0;
    double max_dd  = 0.0;

    for (double p : pnl) {
        cum  += p;
        if (cum > peak) peak = cum;
        double dd = peak - cum;
        if (dd > max_dd) max_dd = dd;
    }
    return max_dd;
}

double Reporting::compute_win_rate(const std::vector<Trade>&  trades,
                                   const std::vector<double>& pnl) const {
    // Win rate = profitable closing trades / total closing trades.
    // Only SELL (ASK) trades realise P&L in a long-only strategy;
    // BUY trades are excluded from the denominator.
    int closing_trades = 0;
    int winning_trades = 0;
    for (std::size_t i = 0; i < trades.size() && i < pnl.size(); ++i) {
        if (trades[i].side == Side::ASK) {
            ++closing_trades;
            if (pnl[i] > 0.0) ++winning_trades;
        }
    }
    if (closing_trades == 0) return 0.0;
    return static_cast<double>(winning_trades) / closing_trades;
}
