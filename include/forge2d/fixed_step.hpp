/**
 * File: fixed_step.hpp
 * Purpose: Convert variable wall-clock durations into bounded deterministic simulation ticks.
 * Symbols and line locations: see docs/code-index.md; FixedStepClock prevents spiral-of-death work.
 */
#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <stdexcept>

namespace forge2d {

class FixedStepClock {
  public:
    explicit FixedStepClock(std::chrono::nanoseconds tick_duration,
                            std::size_t max_ticks_per_frame = 8U)
        : tick_duration_(tick_duration), max_ticks_per_frame_(max_ticks_per_frame) {
        if (tick_duration_ <= std::chrono::nanoseconds::zero() || max_ticks_per_frame_ == 0U) {
            throw std::invalid_argument("fixed-step configuration must be positive");
        }
    }

    [[nodiscard]] std::size_t consume(std::chrono::nanoseconds elapsed) noexcept {
        const auto non_negative = std::max(elapsed, std::chrono::nanoseconds::zero());
        accumulator_ += non_negative;
        const auto available = static_cast<std::size_t>(accumulator_ / tick_duration_);
        const auto ticks = std::min(available, max_ticks_per_frame_);
        accumulator_ -= tick_duration_ * static_cast<std::int64_t>(ticks);
        if (available > max_ticks_per_frame_) {
            dropped_ticks_ += available - max_ticks_per_frame_;
            accumulator_ %= tick_duration_;
        }
        return ticks;
    }

    [[nodiscard]] double interpolation_alpha() const noexcept {
        return static_cast<double>(accumulator_.count()) /
               static_cast<double>(tick_duration_.count());
    }
    [[nodiscard]] std::size_t dropped_ticks() const noexcept {
        return dropped_ticks_;
    }

  private:
    std::chrono::nanoseconds tick_duration_{};
    std::chrono::nanoseconds accumulator_{};
    std::size_t max_ticks_per_frame_{};
    std::size_t dropped_ticks_{};
};

} // namespace forge2d
