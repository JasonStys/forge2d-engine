/**
 * File: entity.hpp
 * Purpose: Define generation-safe entity identifiers, lifecycle allocation, and sparse-set storage.
 * Symbols and line locations: see docs/code-index.md; SparseSet provides expected O(1) access.
 */
#pragma once

#include <compare>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace forge2d {

/** Stable handle whose generation prevents stale references after index reuse. */
struct Entity {
    std::uint32_t index{};
    std::uint32_t generation{};

    [[nodiscard]] constexpr std::uint64_t packed() const noexcept {
        return (static_cast<std::uint64_t>(generation) << 32U) | index;
    }

    auto operator<=>(const Entity&) const = default;
};

/** Allocate and retire generation-safe entity identifiers. */
class EntityPool {
  public:
    [[nodiscard]] Entity create();
    [[nodiscard]] bool destroy(Entity entity) noexcept;
    [[nodiscard]] bool valid(Entity entity) const noexcept;
    [[nodiscard]] std::size_t live_count() const noexcept {
        return live_count_;
    }

  private:
    static constexpr std::uint32_t no_free_index = std::numeric_limits<std::uint32_t>::max();

    std::vector<std::uint32_t> generations_{};
    std::vector<std::uint8_t> alive_{};
    std::vector<std::uint32_t> next_free_{};
    std::uint32_t free_head_{no_free_index};
    std::size_t live_count_{};
};

/** Dense component storage indexed through an entity-index sparse array. */
template <typename Component> class SparseSet {
  public:
    static_assert(std::is_nothrow_move_constructible_v<Component> &&
                      std::is_nothrow_move_assignable_v<Component>,
                  "SparseSet components must support non-throwing moves");
    static constexpr std::uint32_t missing = std::numeric_limits<std::uint32_t>::max();

    [[nodiscard]] bool contains(Entity entity) const noexcept {
        if (entity.index >= sparse_.size()) {
            return false;
        }
        const auto dense_index = sparse_[entity.index];
        return dense_index != missing && dense_index < entities_.size() &&
               entities_[dense_index] == entity;
    }

    Component& insert_or_assign(Entity entity, Component component) {
        ensure_sparse(entity.index);
        if (contains(entity)) {
            components_[sparse_[entity.index]] = std::move(component);
            return components_[sparse_[entity.index]];
        }
        if (sparse_[entity.index] != missing) {
            static_cast<void>(erase(entities_[sparse_[entity.index]]));
        }
        const auto dense_index = static_cast<std::uint32_t>(entities_.size());
        components_.push_back(std::move(component));
        try {
            entities_.push_back(entity);
        } catch (...) {
            components_.pop_back();
            throw;
        }
        sparse_[entity.index] = dense_index;
        return components_.back();
    }

    [[nodiscard]] bool erase(Entity entity) noexcept {
        if (!contains(entity)) {
            return false;
        }
        const auto dense_index = sparse_[entity.index];
        const auto last_index = static_cast<std::uint32_t>(entities_.size() - 1U);
        if (dense_index != last_index) {
            entities_[dense_index] = entities_[last_index];
            components_[dense_index] = std::move(components_[last_index]);
            sparse_[entities_[dense_index].index] = dense_index;
        }
        entities_.pop_back();
        components_.pop_back();
        sparse_[entity.index] = missing;
        return true;
    }

    Component& get(Entity entity) {
        if (!contains(entity)) {
            throw std::out_of_range("component does not exist for entity");
        }
        return components_[sparse_[entity.index]];
    }

    const Component& get(Entity entity) const {
        if (!contains(entity)) {
            throw std::out_of_range("component does not exist for entity");
        }
        return components_[sparse_[entity.index]];
    }

    [[nodiscard]] const std::vector<Entity>& entities() const noexcept {
        return entities_;
    }
    [[nodiscard]] std::vector<Component>& components() noexcept {
        return components_;
    }
    [[nodiscard]] const std::vector<Component>& components() const noexcept {
        return components_;
    }
    [[nodiscard]] std::size_t size() const noexcept {
        return components_.size();
    }

  private:
    void ensure_sparse(std::uint32_t index) {
        if (index >= sparse_.size()) {
            sparse_.resize(static_cast<std::size_t>(index) + 1U, missing);
        }
    }

    std::vector<std::uint32_t> sparse_{};
    std::vector<Entity> entities_{};
    std::vector<Component> components_{};
};

} // namespace forge2d
