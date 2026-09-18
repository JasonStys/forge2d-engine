/**
 * File: collision.hpp
 * Purpose: Declare deterministic AABB broadphase and brute-force reference collision queries.
 * Symbols and line locations: see docs/code-index.md; CollisionPair output is sorted and unique.
 */
#pragma once

#include "forge2d/entity.hpp"
#include "forge2d/math.hpp"

#include <cstdint>
#include <vector>

namespace forge2d {

struct CollisionItem {
    Entity entity{};
    Aabb bounds{};
};

struct CollisionPair {
    Entity first{};
    Entity second{};

    auto operator<=>(const CollisionPair&) const = default;
};

class SpatialHash {
  public:
    explicit SpatialHash(std::int32_t cell_size);

    [[nodiscard]] std::vector<CollisionPair> query(const std::vector<CollisionItem>& items) const;
    [[nodiscard]] std::int32_t cell_size() const noexcept {
        return cell_size_;
    }

  private:
    std::int32_t cell_size_{};
};

[[nodiscard]] std::vector<CollisionPair>
brute_force_collisions(const std::vector<CollisionItem>& items);

} // namespace forge2d
