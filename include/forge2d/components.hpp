/**
 * File: components.hpp
 * Purpose: Define compact data-only simulation and rendering components.
 * Symbols and line locations: see docs/code-index.md; World owns these through sparse sets.
 */
#pragma once

#include "forge2d/math.hpp"

#include <bit>
#include <compare>
#include <cstdint>

namespace forge2d {

/** Jason Stys: one category bit and a 32-bit inclusion mask; zero mask disables matching. */
struct CollisionFilter {
    std::uint32_t category{1U};
    std::uint32_t mask{0xFFFFFFFFU};
    [[nodiscard]] bool valid() const noexcept {
        return std::has_single_bit(category);
    }
    auto operator<=>(const CollisionFilter&) const = default;
};

struct Transform {
    Vec2i position{};
    auto operator<=>(const Transform&) const = default;
};

struct KinematicBody {
    Vec2i velocity{};
    Vec2i half_extent{250, 250};
    CollisionFilter filter{};
    auto operator<=>(const KinematicBody&) const = default;
};

struct Sprite {
    std::uint32_t rgba{0xFFFFFFFFU};
    std::int16_t layer{};
    auto operator<=>(const Sprite&) const = default;
};

struct InputFrame {
    std::int8_t move_x{};
    std::int8_t move_y{};
    bool action{};

    auto operator<=>(const InputFrame&) const = default;
};

} // namespace forge2d
