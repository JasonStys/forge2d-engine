/**
 * File: benchmark.cpp
 * Purpose: Enforce generous regression budgets for sparse storage, collision, and fixed ticks.
 * Symbols and line locations: see docs/code-index.md; main emits machine-readable JSON evidence.
 */
#include "forge2d/collision.hpp"
#include "forge2d/entity.hpp"
#include "forge2d/world.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>
#include <random>
#include <utility>
#include <vector>

namespace {

template <typename Function> [[nodiscard]] double milliseconds(Function&& function) {
    const auto started = std::chrono::steady_clock::now();
    std::forward<Function>(function)();
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started)
        .count();
}

} // namespace

int main() { // NOLINT(bugprone-exception-escape)
    constexpr std::size_t entity_count = 20'000U;
    constexpr double storage_budget_ms = 1'000.0;
    constexpr double collision_budget_ms = 2'500.0;
    constexpr double simulation_budget_ms = 2'500.0;

    forge2d::EntityPool pool;
    forge2d::SparseSet<std::uint64_t> components;
    std::vector<forge2d::Entity> entities;
    entities.reserve(entity_count);
    const auto storage_ms = milliseconds([&] {
        for (std::size_t index = 0; index < entity_count; ++index) {
            const auto entity = pool.create();
            entities.push_back(entity);
            components.insert_or_assign(entity, entity.packed() * 3U);
        }
        for (const auto entity : entities) {
            components.get(entity) += 1U;
        }
        for (std::size_t index = 0; index < entities.size(); index += 2U) {
            static_cast<void>(components.erase(entities[index]));
        }
    });

    std::mt19937 random{0xB3AC2026U};
    std::uniform_int_distribution<std::int32_t> coordinate{100, 99'900};
    std::vector<forge2d::CollisionItem> items;
    items.reserve(entity_count);
    for (std::uint32_t index = 0; index < entity_count; ++index) {
        items.push_back({{index, 0U}, {{coordinate(random), coordinate(random)}, {35, 35}}});
    }
    forge2d::SpatialHash spatial_hash{500};
    std::size_t pair_count{};
    const auto collision_ms = milliseconds([&] { pair_count = spatial_hash.query(items).size(); });

    forge2d::World world{{0, 0}, {100'000, 100'000}, 500};
    for (std::size_t index = 0; index < 5'000U; ++index) {
        static_cast<void>(world.spawn({{coordinate(random), coordinate(random)}},
                                      {{static_cast<std::int32_t>(index % 7U) - 3, 2}, {30, 30}},
                                      {0xFFFFFFFFU, 0}, index == 0U));
    }
    const auto simulation_ms = milliseconds([&] {
        for (std::size_t tick = 0; tick < 240U; ++tick) {
            world.step({1, 0, false});
        }
    });
    const auto passed = storage_ms <= storage_budget_ms && collision_ms <= collision_budget_ms &&
                        simulation_ms <= simulation_budget_ms;
    std::cout << "{\n"
              << "  \"entities\": " << entity_count << ",\n"
              << "  \"collision_pairs\": " << pair_count << ",\n"
              << "  \"storage_ms\": " << storage_ms << ",\n"
              << "  \"collision_ms\": " << collision_ms << ",\n"
              << "  \"simulation_ms\": " << simulation_ms << ",\n"
              << "  \"budgets_ms\": {\"storage\": " << storage_budget_ms
              << ", \"collision\": " << collision_budget_ms
              << ", \"simulation\": " << simulation_budget_ms << "},\n"
              << "  \"passed\": " << (passed ? "true" : "false") << "\n"
              << "}\n";
    return passed ? 0 : 1;
}
