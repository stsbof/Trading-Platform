#pragma once
#include "Types.h"
#include <variant>

// C++17: all feed events in a single discriminated union
using FeedEvent = std::variant<OrderAdd, OrderRemove>;
