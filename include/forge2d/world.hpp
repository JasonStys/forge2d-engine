/**
 * File: world.hpp
 * Purpose: Own deterministic entities/components, advance fixed ticks, and expose snapshots.
 * Symbols and line locations: see docs/code-index.md; World is the engine's simulation boundary.
 */
#pragma once

#include "forge2d/collision.hpp"
#include "forge2d/components.hpp"
#include "forge2d/entity.hpp"
#include "forge2d/replay.hpp"
#include "forge2d/scene.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace forge2d {

struct SnapshotEntity {
    Entity entity{};
    Transform transform{};
    KinematicBody body{};
    Sprite sprite{};
    bool player{};

    auto operator<=>(const SnapshotEntity&) const = default;
};

class World {
  public:
    explicit World(Vec2i minimum = {}, Vec2i maximum = {10'000, 10'000},
                   std::int32_t collision_cell_size = 1'000);

    [[nodiscard]] Entity spawn(Transform transform, KinematicBody body, Sprite sprite,
                               bool player = false);
    [[nodiscard]] bool destroy(Entity entity) noexcept;
    [[nodiscard]] bool valid(Entity entity) const noexcept {
        return entities_.valid(entity);
    }
    void step(InputFrame input);

    [[nodiscard]] std::vector<SnapshotEntity> snapshot() const;
    [[nodiscard]] std::vector<CollisionPair> collision_pairs() const;
    [[nodiscard]] std::uint64_t state_hash() const;
    [[nodiscard]] std::uint64_t tick() const noexcept {
        return tick_;
    }
    [[nodiscard]] std::size_t entity_count() const noexcept {
        return entities_.live_count();
    }
    [[nodiscard]] std::optional<Entity> player() const noexcept {
        return player_;
    }

    static World from_scene(const Scene& scene);

  private:
    [[nodiscard]] std::vector<CollisionItem> collision_items() const;

    EntityPool entities_{};
    SparseSet<Transform> transforms_{};
    SparseSet<KinematicBody> bodies_{};
    SparseSet<Sprite> sprites_{};
    std::optional<Entity> player_{};
    Vec2i minimum_{};
    Vec2i maximum_{};
    SpatialHash collision_grid_;
    std::uint64_t tick_{};
};

[[nodiscard]] std::uint64_t run_replay(World world, const Replay& replay);

} // namespace forge2d
