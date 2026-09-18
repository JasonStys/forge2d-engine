/**
 * File: collision.cpp
 * Purpose: Implement deterministic spatial-hash broadphase and exact AABB pair filtering.
 * Symbols and line locations: see docs/code-index.md; query de-duplicates across occupied cells.
 */
#include "forge2d/collision.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>

namespace forge2d {
namespace {

constexpr std::size_t max_query_items = 100'000U;
constexpr std::int64_t max_cells_per_item = 4'096;

[[nodiscard]] std::int64_t floor_div(std::int64_t value, std::int64_t divisor) noexcept {
    auto quotient = value / divisor;
    const auto remainder = value % divisor;
    if (remainder != 0 && value < 0) {
        --quotient;
    }
    return quotient;
}

[[nodiscard]] CollisionPair normalized_pair(Entity left, Entity right) noexcept {
    return left < right ? CollisionPair{left, right} : CollisionPair{right, left};
}

} // namespace

SpatialHash::SpatialHash(std::int32_t cell_size) : cell_size_(cell_size) {
    if (cell_size_ <= 0) {
        throw std::invalid_argument("spatial-hash cell size must be positive");
    }
}

std::vector<CollisionPair> SpatialHash::query(const std::vector<CollisionItem>& items) const {
    if (items.size() > max_query_items) {
        throw std::length_error("spatial-hash query exceeds the item limit");
    }
    std::map<std::pair<std::int64_t, std::int64_t>, std::vector<std::size_t>> cells;
    for (std::size_t item_index = 0; item_index < items.size(); ++item_index) {
        const auto& bounds = items[item_index].bounds;
        if (bounds.half_extent.x <= 0 || bounds.half_extent.y <= 0) {
            throw std::invalid_argument("collision half extents must be positive");
        }
        const auto min_x = floor_div(bounds.left(), cell_size_);
        const auto max_x = floor_div(bounds.right() - 1, cell_size_);
        const auto min_y = floor_div(bounds.top(), cell_size_);
        const auto max_y = floor_div(bounds.bottom() - 1, cell_size_);
        const auto span_x = max_x - min_x + 1;
        const auto span_y = max_y - min_y + 1;
        if (span_x > max_cells_per_item || span_y > max_cells_per_item ||
            span_x * span_y > max_cells_per_item) {
            throw std::length_error("collision item exceeds the occupied-cell limit");
        }
        for (auto cell_y = min_y; cell_y <= max_y; ++cell_y) {
            for (auto cell_x = min_x; cell_x <= max_x; ++cell_x) {
                cells[{cell_x, cell_y}].push_back(item_index);
            }
        }
    }

    std::set<CollisionPair> pairs;
    for (const auto& [cell, members] : cells) {
        static_cast<void>(cell);
        for (std::size_t left_index = 0; left_index < members.size(); ++left_index) {
            for (std::size_t right_index = left_index + 1U; right_index < members.size();
                 ++right_index) {
                const auto& left = items[members[left_index]];
                const auto& right = items[members[right_index]];
                if (intersects(left.bounds, right.bounds)) {
                    pairs.insert(normalized_pair(left.entity, right.entity));
                }
            }
        }
    }
    return {pairs.begin(), pairs.end()};
}

std::vector<CollisionPair> brute_force_collisions(const std::vector<CollisionItem>& items) {
    std::vector<CollisionPair> pairs;
    for (std::size_t left_index = 0; left_index < items.size(); ++left_index) {
        for (std::size_t right_index = left_index + 1U; right_index < items.size(); ++right_index) {
            if (intersects(items[left_index].bounds, items[right_index].bounds)) {
                pairs.push_back(
                    normalized_pair(items[left_index].entity, items[right_index].entity));
            }
        }
    }
    std::sort(pairs.begin(), pairs.end());
    pairs.erase(std::unique(pairs.begin(), pairs.end()), pairs.end());
    return pairs;
}

} // namespace forge2d
