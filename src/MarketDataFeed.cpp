#include "MarketDataFeed.h"
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <vector>

MarketDataFeed::MarketDataFeed(const std::string& csv_path) {
    file_.open(csv_path);
    if (!file_.is_open())
        throw std::runtime_error("MarketDataFeed: cannot open file '" + csv_path + "'");
}

bool MarketDataFeed::has_more() const {
    return file_.good() && !file_.eof();
}

std::optional<FeedEvent> MarketDataFeed::next_event() {
    std::string line;
    while (std::getline(file_, line)) {
        ++line_number_;

        // Skip header
        if (!header_skipped_) {
            header_skipped_ = true;
            continue;
        }

        // Skip blank lines
        if (line.empty() || std::all_of(line.begin(), line.end(), ::isspace))
            continue;

        return parse_line(line);
    }
    return std::nullopt;
}

FeedEvent MarketDataFeed::parse_line(const std::string& line) {
    auto error = [&](const std::string& msg) {
        throw std::runtime_error(
            "MarketDataFeed line " + std::to_string(line_number_) + ": " + msg +
            " (raw: '" + line + "')");
    };

    std::vector<std::string> tokens;
    std::istringstream ss(line);
    std::string tok;
    while (std::getline(ss, tok, ','))
        tokens.push_back(tok);

    if (tokens.size() < 3)
        error("not enough columns");

    int64_t timestamp{};
    try { timestamp = std::stoll(tokens[0]); }
    catch (...) { error("invalid timestamp '" + tokens[0] + "'"); }

    const std::string& event_type = tokens[1];

    if (event_type == "REMOVE") {
        if (tokens.size() < 2)
            error("REMOVE event missing order_id");
        int64_t oid{};
        try { oid = std::stoll(tokens[2]); }
        catch (...) { error("invalid order_id '" + tokens[2] + "'"); }
        return OrderRemove{oid};
    }

    if (event_type == "ADD") {
        if (tokens.size() < 6)
            error("ADD event requires 6 columns");

        int64_t oid{};
        try { oid = std::stoll(tokens[2]); }
        catch (...) { error("invalid order_id '" + tokens[2] + "'"); }

        const std::string& side_str = tokens[3];
        Side side{};
        if      (side_str == "BID") side = Side::BID;
        else if (side_str == "ASK") side = Side::ASK;
        else error("invalid side '" + side_str + "'");

        double price{};
        try { price = std::stod(tokens[4]); }
        catch (...) { error("invalid price '" + tokens[4] + "'"); }
        if (price <= 0) error("price must be positive");

        int64_t qty{};
        try { qty = std::stoll(tokens[5]); }
        catch (...) { error("invalid quantity '" + tokens[5] + "'"); }
        if (qty <= 0) error("quantity must be positive");

        return OrderAdd{oid, side, price, qty, timestamp};
    }

    error("unknown event_type '" + event_type + "'");
    // unreachable
    return OrderRemove{-1};
}
