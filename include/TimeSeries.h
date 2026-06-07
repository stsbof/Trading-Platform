#pragma once
#include <deque>
#include <numeric>
#include <stdexcept>
#include <cmath>

// Rolling window time series — template used for price history and returns
template<typename T>
class TimeSeries {
public:
    explicit TimeSeries(std::size_t max_size) : max_size_(max_size) {}

    void push(T value) {
        data_.push_back(value);
        if (data_.size() > max_size_)
            data_.pop_front();
    }

    std::size_t size()     const { return data_.size(); }
    bool        full()     const { return data_.size() == max_size_; }
    T           back()     const { return data_.back(); }
    T           front()    const { return data_.front(); }

    const std::deque<T>& data() const { return data_; }

    T mean() const {
        if (data_.empty()) throw std::runtime_error("TimeSeries is empty");
        return std::accumulate(data_.begin(), data_.end(), T{}) / static_cast<T>(data_.size());
    }

    T stddev() const {
        if (data_.size() < 2) throw std::runtime_error("TimeSeries needs at least 2 elements");
        T m = mean();
        T var{};
        for (const auto& v : data_)
            var += (v - m) * (v - m);
        return std::sqrt(var / static_cast<T>(data_.size()));
    }

private:
    std::size_t   max_size_;
    std::deque<T> data_;
};
