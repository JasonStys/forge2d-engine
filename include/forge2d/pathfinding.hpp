/**
 * File: pathfinding.hpp
 * Purpose: Declare a bounded grid and deterministic four-neighbor A* pathfinding.
 * Symbols and line locations: see docs/code-index.md; GridPoint and Grid define path contracts.
 */
#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace forge2d {

struct GridPoint {
    std::int32_t x{};
    std::int32_t y{};

    auto operator<=>(const GridPoint&) const = default;
};

class Grid {
  public:
    Grid(std::int32_t width, std::int32_t height);

    [[nodiscard]] std::int32_t width() const noexcept {
        return width_;
    }
    [[nodiscard]] std::int32_t height() const noexcept {
        return height_;
    }
    [[nodiscard]] bool in_bounds(GridPoint point) const noexcept;
    [[nodiscard]] bool blocked(GridPoint point) const;
    void set_blocked(GridPoint point, bool blocked);

  private:
    [[nodiscard]] std::size_t index(GridPoint point) const;

    std::int32_t width_{};
    std::int32_t height_{};
    std::vector<std::uint8_t> blocked_{};
};

[[nodiscard]] std::vector<GridPoint> find_path(const Grid& grid, GridPoint start, GridPoint goal);

} // namespace forge2d
