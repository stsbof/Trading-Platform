#pragma once
#include "Portfolio.h"
#include <string>

class Reporting {
public:
    // risk_free_rate: per-period risk-free rate (same frequency as the return series,
    //                 default 0). With √252 annualisation this corresponds to a
    //                 daily rate if each closing trade ≈ one trading day.
    explicit Reporting(double risk_free_rate = 0.0);

    void generate(const Portfolio& portfolio, const std::string& strategy_name) const;

private:
    double risk_free_rate_;

    double compute_sharpe(const std::vector<double>& pnl_series,
                          double initial_equity) const;
    double compute_max_drawdown(const std::vector<double>& pnl_series) const;
    double compute_win_rate(const std::vector<Trade>& trades,
                            const std::vector<double>& pnl_series) const;
};
