/**
 * File: math.hpp
 * Purpose: Provide deterministic integer simulation vectors and axis-aligned bounding boxes.
 * Symbols and line locations: see docs/code-index.md; all simulation units are subpixels per tick.
 */
#pragma once

#include <algorithm>
#include <compare>
#include <cstdint>
#include <limits>

namespace forge2d {

struct Vec2i {
    std::int32_t x{};
    std::int32_t y{};

    constexpr Vec2i& operator+=(Vec2i other) noexcept {
        x = static_cast<std::int32_t>(
            std::clamp(static_cast<std::int64_t>(x) + other.x,
                       static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min()),
                       static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max())));
        y = static_cast<std::int32_t>(
            std::clamp(static_cast<std::int64_t>(y) + other.y,
                       static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min()),
                       static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max())));
        return *this;
    }
    auto operator<=>(const Vec2i&) const = default;
};

[[nodiscard]] constexpr Vec2i operator+(Vec2i left, Vec2i right) noexcept {
    left += right;
    return left;
}

[[nodiscard]] constexpr Vec2i operator-(Vec2i left, Vec2i right) noexcept {
    const auto saturating_subtract = [](std::int32_t lhs, std::int32_t rhs) {
        return static_cast<std::int32_t>(
            std::clamp(static_cast<std::int64_t>(lhs) - rhs,
                       static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min()),
                       static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max())));
    };
    return {saturating_subtract(left.x, right.x), saturating_subtract(left.y, right.y)};
}

[[nodiscard]] constexpr Vec2i operator*(Vec2i value, std::int32_t scalar) noexcept {
    const auto saturating_multiply = [scalar](std::int32_t coordinate) {
        return static_cast<std::int32_t>(
            std::clamp(static_cast<std::int64_t>(coordinate) * scalar,
                       static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min()),
                       static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max())));
    };
    return {saturating_multiply(value.x), saturating_multiply(value.y)};
}

[[nodiscard]] constexpr std::uint64_t length_squared(Vec2i value) noexcept {
    const auto magnitude = [](std::int32_t coordinate) {
        const auto wide = static_cast<std::int64_t>(coordinate);
        return static_cast<std::uint64_t>(wide < 0 ? -wide : wide);
    };
    const auto x = magnitude(value.x);
    const auto y = magnitude(value.y);
    return x * x + y * y;
}

struct Aabb {
    Vec2i center{};
    Vec2i half_extent{};

    [[nodiscard]] constexpr std::int64_t left() const noexcept {
        return static_cast<std::int64_t>(center.x) - half_extent.x;
    }
    [[nodiscard]] constexpr std::int64_t right() const noexcept {
        return static_cast<std::int64_t>(center.x) + half_extent.x;
    }
    [[nodiscard]] constexpr std::int64_t top() const noexcept {
        return static_cast<std::int64_t>(center.y) - half_extent.y;
    }
    [[nodiscard]] constexpr std::int64_t bottom() const noexcept {
        return static_cast<std::int64_t>(center.y) + half_extent.y;
    }

    auto operator<=>(const Aabb&) const = default;
};

[[nodiscard]] constexpr bool intersects(const Aabb& left, const Aabb& right) noexcept {
    return left.left() < right.right() && left.right() > right.left() &&
           left.top() < right.bottom() && left.bottom() > right.top();
}

[[nodiscard]] constexpr std::int32_t clamp_axis(std::int32_t value, std::int32_t low,
                                                std::int32_t high) noexcept {
    return std::clamp(value, low, high);
}

} // namespace forge2d
