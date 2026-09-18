/**
 * File: pathfinding.cpp
 * Purpose: Implement bounded deterministic A* over a four-neighbor rectangular grid.
 * Symbols and line locations: see docs/code-index.md; stable tie-breaking makes paths replayable.
 */
#include "forge2d/pathfinding.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <limits>
#include <queue>
#include <stdexcept>

namespace forge2d {
namespace {

constexpr std::int32_t max_dimension = 4'096;
constexpr std::size_t max_cells = 4'000'000U;

[[nodiscard]] std::int32_t heuristic(GridPoint left, GridPoint right) noexcept {
    return std::abs(left.x - right.x) + std::abs(left.y - right.y);
}

struct OpenNode {
    std::int32_t score{};
    std::int32_t cost{};
    std::size_t index{};
};

struct HigherPriority {
    bool operator()(const OpenNode& left, const OpenNode& right) const noexcept {
        if (left.score != right.score) {
            return left.score > right.score;
        }
        if (left.cost != right.cost) {
            return left.cost > right.cost;
        }
        return left.index > right.index;
    }
};

} // namespace

Grid::Grid(std::int32_t width, std::int32_t height) : width_(width), height_(height) {
    if (width_ <= 0 || height_ <= 0 || width_ > max_dimension || height_ > max_dimension) {
        throw std::invalid_argument("grid dimensions must be in the range 1..4096");
    }
    const auto cell_count = static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_);
    if (cell_count > max_cells) {
        throw std::invalid_argument("grid exceeds the four-million-cell safety limit");
    }
    blocked_.resize(cell_count, 0U);
}

bool Grid::in_bounds(GridPoint point) const noexcept {
    return point.x >= 0 && point.y >= 0 && point.x < width_ && point.y < height_;
}

std::size_t Grid::index(GridPoint point) const {
    if (!in_bounds(point)) {
        throw std::out_of_range("grid point is outside the grid");
    }
    return static_cast<std::size_t>(point.y) * static_cast<std::size_t>(width_) +
           static_cast<std::size_t>(point.x);
}

bool Grid::blocked(GridPoint point) const {
    return blocked_[index(point)] != 0U;
}

void Grid::set_blocked(GridPoint point, bool blocked) {
    blocked_[index(point)] = blocked ? 1U : 0U;
}

std::vector<GridPoint> find_path(const Grid& grid, GridPoint start, GridPoint goal) {
    if (!grid.in_bounds(start) || !grid.in_bounds(goal) || grid.blocked(start) ||
        grid.blocked(goal)) {
        return {};
    }
    const auto width = static_cast<std::size_t>(grid.width());
    const auto cell_count = width * static_cast<std::size_t>(grid.height());
    const auto to_index = [width](GridPoint point) {
        return static_cast<std::size_t>(point.y) * width + static_cast<std::size_t>(point.x);
    };
    const auto to_point = [width](std::size_t index) {
        return GridPoint{static_cast<std::int32_t>(index % width),
                         static_cast<std::int32_t>(index / width)};
    };
    const auto start_index = to_index(start);
    const auto goal_index = to_index(goal);
    const auto infinity = std::numeric_limits<std::int32_t>::max();
    std::vector<std::int32_t> costs(cell_count, infinity);
    std::vector<std::size_t> parent(cell_count, cell_count);
    std::priority_queue<OpenNode, std::vector<OpenNode>, HigherPriority> open;
    costs[start_index] = 0;
    open.push({heuristic(start, goal), 0, start_index});
    constexpr std::array<GridPoint, 4> directions{{{1, 0}, {0, 1}, {-1, 0}, {0, -1}}};

    while (!open.empty()) {
        const auto current = open.top();
        open.pop();
        if (current.cost != costs[current.index]) {
            continue;
        }
        if (current.index == goal_index) {
            break;
        }
        const auto point = to_point(current.index);
        for (const auto direction : directions) {
            const auto neighbor = GridPoint{point.x + direction.x, point.y + direction.y};
            if (!grid.in_bounds(neighbor) || grid.blocked(neighbor)) {
                continue;
            }
            const auto neighbor_index = to_index(neighbor);
            const auto next_cost = current.cost + 1;
            if (next_cost < costs[neighbor_index]) {
                costs[neighbor_index] = next_cost;
                parent[neighbor_index] = current.index;
                open.push({next_cost + heuristic(neighbor, goal), next_cost, neighbor_index});
            }
        }
    }
    if (costs[goal_index] == infinity) {
        return {};
    }
    std::vector<GridPoint> path;
    for (auto current = goal_index;; current = parent[current]) {
        path.push_back(to_point(current));
        if (current == start_index) {
            break;
        }
    }
    std::reverse(path.begin(), path.end());
    return path;
}

} // namespace forge2d
