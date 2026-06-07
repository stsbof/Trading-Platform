#pragma once
#include "Events.h"
#include <string>
#include <fstream>
#include <optional>

class MarketDataFeed {
public:
    explicit MarketDataFeed(const std::string& csv_path);

    // Returns the next event from the CSV, or nullopt when exhausted
    std::optional<FeedEvent> next_event();

    bool has_more() const;

private:
    std::ifstream file_;
    int64_t       line_number_{0};
    bool          header_skipped_{false};

    FeedEvent parse_line(const std::string& line);
};
