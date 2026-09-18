/**
 * File: world.cpp
 * Purpose: Implement generation-safe entity lifecycle, deterministic ticks, snapshots, and hashes.
 * Symbols and line locations: see docs/code-index.md; World::step performs no wall-clock reads.
 */
#include "forge2d/world.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>

namespace forge2d {
namespace {

constexpr std::int32_t player_speed = 140;
constexpr std::uint64_t fnv_offset = 14'695'981'039'346'656'037ULL;
constexpr std::uint64_t fnv_prime = 1'099'511'628'211ULL;

void hash_u64(std::uint64_t& hash, std::uint64_t value) noexcept {
    for (std::size_t byte = 0; byte < 8U; ++byte) {
        hash ^= (value >> (byte * 8U)) & 0xFFU;
        hash *= fnv_prime;
    }
}

[[nodiscard]] std::uint64_t signed_bits(std::int32_t value) noexcept {
    return static_cast<std::uint64_t>(static_cast<std::uint32_t>(value));
}

[[nodiscard]] std::int32_t saturating_int32(std::int64_t value) noexcept {
    return static_cast<std::int32_t>(
        std::clamp(value, static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min()),
                   static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max())));
}

} // namespace

Entity EntityPool::create() {
    std::uint32_t index{};
    if (free_head_ == no_free_index) {
        if (generations_.size() >= std::numeric_limits<std::uint32_t>::max()) {
            throw std::overflow_error("entity index space exhausted");
        }
        index = static_cast<std::uint32_t>(generations_.size());
        if (generations_.capacity() == generations_.size() || alive_.capacity() == alive_.size() ||
            next_free_.capacity() == next_free_.size()) {
            constexpr std::size_t initial_capacity = 8U;
            const auto growth_capacity =
                generations_.size() <= std::numeric_limits<std::size_t>::max() / 2U
                    ? generations_.size() * 2U
                    : generations_.size() + 1U;
            const auto required_capacity = std::max(initial_capacity, growth_capacity);
            generations_.reserve(required_capacity);
            alive_.reserve(required_capacity);
            next_free_.reserve(required_capacity);
        }
        generations_.push_back(0U);
        alive_.push_back(1U);
        next_free_.push_back(no_free_index);
    } else {
        index = free_head_;
        free_head_ = next_free_[index];
        next_free_[index] = no_free_index;
        alive_[index] = 1U;
    }
    ++live_count_;
    return {index, generations_[index]};
}

bool EntityPool::destroy(Entity entity) noexcept {
    if (!valid(entity)) {
        return false;
    }
    alive_[entity.index] = 0U;
    --live_count_;
    if (generations_[entity.index] != std::numeric_limits<std::uint32_t>::max()) {
        ++generations_[entity.index];
        next_free_[entity.index] = free_head_;
        free_head_ = entity.index;
    }
    return true;
}

bool EntityPool::valid(Entity entity) const noexcept {
    return entity.index < generations_.size() && alive_[entity.index] &&
           generations_[entity.index] == entity.generation;
}

World::World(Vec2i minimum, Vec2i maximum, std::int32_t collision_cell_size)
    : minimum_(minimum), maximum_(maximum), collision_grid_(collision_cell_size) {
    if (minimum_.x >= maximum_.x || minimum_.y >= maximum_.y) {
        throw std::invalid_argument("world bounds must have positive area");
    }
}

Entity World::spawn(Transform transform, KinematicBody body, Sprite sprite, bool player) {
    if (body.half_extent.x <= 0 || body.half_extent.y <= 0) {
        throw std::invalid_argument("body half extents must be positive");
    }
    if (player && player_.has_value()) {
        throw std::invalid_argument("world already has a player");
    }
    const auto low_x = static_cast<std::int64_t>(minimum_.x) + body.half_extent.x;
    const auto high_x = static_cast<std::int64_t>(maximum_.x) - body.half_extent.x;
    const auto low_y = static_cast<std::int64_t>(minimum_.y) + body.half_extent.y;
    const auto high_y = static_cast<std::int64_t>(maximum_.y) - body.half_extent.y;
    if (low_x > high_x || low_y > high_y || transform.position.x < low_x ||
        transform.position.x > high_x || transform.position.y < low_y ||
        transform.position.y > high_y) {
        throw std::invalid_argument("body must fit entirely inside the world bounds");
    }
    const auto entity = entities_.create();
    try {
        transforms_.insert_or_assign(entity, transform);
        bodies_.insert_or_assign(entity, body);
        sprites_.insert_or_assign(entity, sprite);
    } catch (...) {
        static_cast<void>(transforms_.erase(entity));
        static_cast<void>(bodies_.erase(entity));
        static_cast<void>(sprites_.erase(entity));
        static_cast<void>(entities_.destroy(entity));
        throw;
    }
    if (player) {
        player_ = entity;
    }
    return entity;
}

bool World::destroy(Entity entity) noexcept {
    if (!entities_.destroy(entity)) {
        return false;
    }
    static_cast<void>(transforms_.erase(entity));
    static_cast<void>(bodies_.erase(entity));
    static_cast<void>(sprites_.erase(entity));
    if (player_ == entity) {
        player_.reset();
    }
    return true;
}

void World::step(InputFrame input) {
    if (input.move_x < -1 || input.move_x > 1 || input.move_y < -1 || input.move_y > 1) {
        throw std::invalid_argument("input axes must be in the range -1..1");
    }
    if (player_.has_value() && bodies_.contains(*player_)) {
        auto& player_body = bodies_.get(*player_);
        player_body.velocity = {static_cast<std::int32_t>(input.move_x) * player_speed,
                                static_cast<std::int32_t>(input.move_y) * player_speed};
    }
    const auto& body_entities = bodies_.entities();
    auto& body_components = bodies_.components();
    for (std::size_t index = 0; index < body_entities.size(); ++index) {
        auto& body = body_components[index];
        auto& transform = transforms_.get(body_entities[index]);
        const auto low_x = static_cast<std::int64_t>(minimum_.x) + body.half_extent.x;
        const auto high_x = static_cast<std::int64_t>(maximum_.x) - body.half_extent.x;
        const auto low_y = static_cast<std::int64_t>(minimum_.y) + body.half_extent.y;
        const auto high_y = static_cast<std::int64_t>(maximum_.y) - body.half_extent.y;
        const auto next_x = static_cast<std::int64_t>(transform.position.x) + body.velocity.x;
        const auto next_y = static_cast<std::int64_t>(transform.position.y) + body.velocity.y;
        transform.position.x = saturating_int32(std::clamp(next_x, low_x, high_x));
        transform.position.y = saturating_int32(std::clamp(next_y, low_y, high_y));
        if (next_x < low_x || next_x > high_x) {
            body.velocity.x = saturating_int32(-static_cast<std::int64_t>(body.velocity.x));
        }
        if (next_y < low_y || next_y > high_y) {
            body.velocity.y = saturating_int32(-static_cast<std::int64_t>(body.velocity.y));
        }
    }
    ++tick_;
}

std::vector<SnapshotEntity> World::snapshot() const {
    std::vector<SnapshotEntity> result;
    result.reserve(transforms_.size());
    for (const auto entity : transforms_.entities()) {
        if (bodies_.contains(entity) && sprites_.contains(entity)) {
            result.push_back({entity, transforms_.get(entity), bodies_.get(entity),
                              sprites_.get(entity), player_ == entity});
        }
    }
    std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) {
        if (left.sprite.layer != right.sprite.layer) {
            return left.sprite.layer < right.sprite.layer;
        }
        return left.entity < right.entity;
    });
    return result;
}

std::vector<CollisionItem> World::collision_items() const {
    std::vector<CollisionItem> result;
    result.reserve(bodies_.size());
    for (const auto entity : bodies_.entities()) {
        if (transforms_.contains(entity)) {
            result.push_back(
                {entity, {transforms_.get(entity).position, bodies_.get(entity).half_extent}});
        }
    }
    return result;
}

std::vector<CollisionPair> World::collision_pairs() const {
    return collision_grid_.query(collision_items());
}

std::uint64_t World::state_hash() const {
    auto hash = fnv_offset;
    hash_u64(hash, tick_);
    for (const auto& value : snapshot()) {
        hash_u64(hash, value.entity.packed());
        hash_u64(hash, signed_bits(value.transform.position.x));
        hash_u64(hash, signed_bits(value.transform.position.y));
        hash_u64(hash, signed_bits(value.body.velocity.x));
        hash_u64(hash, signed_bits(value.body.velocity.y));
        hash_u64(hash, signed_bits(value.body.half_extent.x));
        hash_u64(hash, signed_bits(value.body.half_extent.y));
        hash_u64(hash, value.sprite.rgba);
        hash_u64(hash, static_cast<std::uint16_t>(value.sprite.layer));
        hash_u64(hash, value.player ? 1U : 0U);
    }
    return hash;
}

World World::from_scene(const Scene& scene) {
    World world{scene.minimum, scene.maximum};
    for (const auto& entity : scene.entities) {
        static_cast<void>(world.spawn(entity.transform, entity.body, entity.sprite, entity.player));
    }
    return world;
}

std::uint64_t run_replay(World world, const Replay& replay) {
    for (const auto frame : replay.frames) {
        world.step(frame);
    }
    return world.state_hash();
}

} // namespace forge2d
