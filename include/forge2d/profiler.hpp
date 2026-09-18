/**
 * File: profiler.hpp
 * Purpose: Collect bounded frame timing samples and expose percentile summaries.
 * Symbols and line locations: see docs/code-index.md; FrameProfiler retains a fixed sample window.
 */
#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace forge2d {

struct TimingSummary {
    double mean_microseconds{};
    double p95_microseconds{};
    double maximum_microseconds{};
    std::size_t sample_count{};
};

class FrameProfiler {
  public:
    explicit FrameProfiler(std::size_t capacity) : capacity_(capacity) {
        if (capacity_ == 0U) {
            throw std::invalid_argument("profiler capacity must be positive");
        }
        samples_.reserve(capacity_);
    }

    void record(std::chrono::nanoseconds elapsed) {
        const auto microseconds = static_cast<double>(elapsed.count()) / 1'000.0;
        if (samples_.size() < capacity_) {
            samples_.push_back(microseconds);
        } else {
            samples_[next_] = microseconds;
            next_ = (next_ + 1U) % capacity_;
        }
    }

    [[nodiscard]] TimingSummary summary() const {
        if (samples_.empty()) {
            return {};
        }
        auto sorted = samples_;
        std::sort(sorted.begin(), sorted.end());
        const auto p95_index = std::min(sorted.size() - 1U, sorted.size() * 95U / 100U);
        return {std::accumulate(sorted.begin(), sorted.end(), 0.0) /
                    static_cast<double>(sorted.size()),
                sorted[p95_index], sorted.back(), sorted.size()};
    }

  private:
    std::size_t capacity_{};
    std::size_t next_{};
    std::vector<double> samples_{};
};

} // namespace forge2d
