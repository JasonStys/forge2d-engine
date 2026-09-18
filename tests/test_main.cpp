/**
 * File: test_main.cpp
 * Purpose: Verify engine math, storage, collision, pathfinding, parsing, replay, and rendering.
 * Symbols and line locations: see docs/code-index.md; randomized reference checks use fixed seeds.
 */
#include "forge2d/collision.hpp"
#include "forge2d/entity.hpp"
#include "forge2d/fixed_step.hpp"
#include "forge2d/pathfinding.hpp"
#include "forge2d/profiler.hpp"
#include "forge2d/renderer.hpp"
#include "forge2d/replay.hpp"
#include "forge2d/scene.hpp"
#include "forge2d/world.hpp"

#include "arena.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <functional>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

using Test = std::pair<std::string, std::function<void()>>;

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <typename Function> void require_throws(Function&& function, const std::string& message) {
    try {
        std::forward<Function>(function)();
    } catch (const std::exception&) {
        return;
    }
    throw std::runtime_error(message);
}

[[nodiscard]] forge2d::Scene sample_scene() {
    return {{0, 0},
            {10'000, 10'000},
            {{"player", {{1'000, 1'000}}, {{0, 0}, {200, 200}}, {0x00FF00FFU, 2}, true},
             {"drone", {{8'000, 8'000}}, {{-25, 10}, {250, 250}}, {0xFF0000FFU, 1}, false}}};
}

void test_math_and_fixed_step() {
    require(forge2d::length_squared({3, 4}) == 25, "integer vector length must be exact");
    require(forge2d::intersects({{10, 10}, {5, 5}}, {{14, 10}, {2, 2}}),
            "overlapping boxes must collide");
    require(!forge2d::intersects({{10, 10}, {5, 5}}, {{20, 10}, {5, 5}}),
            "touching edges are not penetration");
    const forge2d::Aabb extreme{{std::numeric_limits<std::int32_t>::min(), 0}, {1, 1}};
    require(extreme.left() ==
                static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min()) - 1,
            "AABB edges must use overflow-safe wide arithmetic");
    require((forge2d::Vec2i{std::numeric_limits<std::int32_t>::max(), 0} + forge2d::Vec2i{1, 0}) ==
                forge2d::Vec2i{std::numeric_limits<std::int32_t>::max(), 0},
            "vector arithmetic must saturate instead of overflowing");
    forge2d::FixedStepClock clock{std::chrono::milliseconds{10}, 3U};
    require(clock.consume(std::chrono::milliseconds{25}) == 2U, "25ms must produce two ticks");
    require(clock.consume(std::chrono::milliseconds{95}) == 3U, "catch-up work must be bounded");
    require(clock.dropped_ticks() == 7U, "excess work must be accounted as dropped ticks");
    require(clock.interpolation_alpha() >= 0.0 && clock.interpolation_alpha() < 1.0,
            "interpolation alpha must remain normalized");
    require_throws([] { static_cast<void>(forge2d::FixedStepClock{std::chrono::nanoseconds{0}}); },
                   "zero-duration fixed steps must be rejected");
}

void test_entity_pool_and_sparse_set_properties() {
    forge2d::EntityPool pool;
    std::vector<forge2d::Entity> entities;
    entities.reserve(256U);
    for (std::size_t index = 0; index < 256U; ++index) {
        entities.push_back(pool.create());
    }
    forge2d::SparseSet<std::int32_t> components;
    std::unordered_map<std::uint64_t, std::int32_t> reference;
    std::mt19937 random{0xF02D2026U};
    std::uniform_int_distribution<std::size_t> entity_distribution{0U, entities.size() - 1U};
    std::uniform_int_distribution<std::int32_t> value_distribution{-10'000, 10'000};
    for (std::size_t operation = 0; operation < 8'000U; ++operation) {
        const auto entity = entities[entity_distribution(random)];
        if ((operation % 3U) == 0U) {
            const auto erased = components.erase(entity);
            const auto expected = reference.erase(entity.packed()) != 0U;
            require(erased == expected, "sparse-set erase must match map semantics");
        } else {
            const auto value = value_distribution(random);
            components.insert_or_assign(entity, value);
            reference[entity.packed()] = value;
        }
        for (const auto candidate : entities) {
            const auto iterator = reference.find(candidate.packed());
            require(components.contains(candidate) == (iterator != reference.end()),
                    "sparse-set presence must match reference map");
            if (iterator != reference.end()) {
                require(components.get(candidate) == iterator->second,
                        "sparse-set value must match reference map");
            }
        }
    }
    const auto stale = entities.front();
    require(pool.destroy(stale), "live entity must be destroyable");
    const auto replacement = pool.create();
    require(replacement.index == stale.index && replacement.generation != stale.generation,
            "reused indices must advance generation");
    require(!pool.valid(stale) && pool.valid(replacement), "stale handles must stay invalid");
}

void test_c_arena_alignment_and_overflow() {
    alignas(64) std::array<std::byte, 256> memory{};
    forge2d_arena arena{};
    forge2d_arena_init(&arena, memory.data(), memory.size());
    auto* first = forge2d_arena_allocate(&arena, 7U, 8U);
    auto* second = forge2d_arena_allocate(&arena, 32U, 32U);
    require(first != nullptr && second != nullptr, "valid arena allocations must succeed");
    require(reinterpret_cast<std::uintptr_t>(second) % 32U == 0U,
            "arena result must honor requested alignment");
    const auto offset = arena.offset;
    require(forge2d_arena_allocate(&arena, 1'024U, 8U) == nullptr,
            "oversized arena allocation must fail");
    require(arena.offset == offset, "failed allocation must not consume storage");
    require(forge2d_arena_allocate(&arena, 8U, 3U) == nullptr,
            "non-power-of-two alignment must fail");
    forge2d_arena_reset(&arena);
    require(arena.offset == 0U && arena.peak >= offset, "reset must preserve high-water evidence");
}

void test_spatial_hash_matches_brute_force() {
    forge2d::SpatialHash spatial_hash{500};
    std::mt19937 random{0xC0111510U};
    std::uniform_int_distribution<std::int32_t> coordinate{-4'000, 4'000};
    std::uniform_int_distribution<std::int32_t> extent{25, 450};
    for (std::size_t scene = 0; scene < 150U; ++scene) {
        std::vector<forge2d::CollisionItem> items;
        items.reserve(50U);
        for (std::uint32_t index = 0; index < 50U; ++index) {
            items.push_back(
                {{index, 0U},
                 {{coordinate(random), coordinate(random)}, {extent(random), extent(random)}}});
        }
        require(spatial_hash.query(items) == forge2d::brute_force_collisions(items),
                "spatial hash must match quadratic reference exactly");
    }
    require_throws([] { static_cast<void>(forge2d::SpatialHash{0}); },
                   "zero-sized spatial cells must be rejected");
    require_throws([&] { static_cast<void>(spatial_hash.query({{{0U, 0U}, {{0, 0}, {0, 10}}}})); },
                   "non-positive collision extents must be rejected");
    require_throws(
        [&] {
            static_cast<void>(spatial_hash.query({{{0U, 0U}, {{0, 0}, {1'500'000, 1'500'000}}}}));
        },
        "a single item must not allocate an unbounded number of cells");
}

void test_astar_returns_valid_shortest_path() {
    forge2d::Grid grid{12, 10};
    for (std::int32_t y = 0; y < grid.height(); ++y) {
        if (y != 6) {
            grid.set_blocked({5, y}, true);
        }
    }
    const auto path = forge2d::find_path(grid, {1, 1}, {10, 8});
    require(!path.empty() && path.front() == forge2d::GridPoint{1, 1} &&
                path.back() == forge2d::GridPoint{10, 8},
            "A* path must include requested endpoints");
    for (std::size_t index = 1; index < path.size(); ++index) {
        const auto delta_x = path[index].x - path[index - 1U].x;
        const auto delta_y = path[index].y - path[index - 1U].y;
        require(std::abs(delta_x) + std::abs(delta_y) == 1 && !grid.blocked(path[index]),
                "A* path must contain adjacent traversable cells");
    }
    require(path.size() == 17U, "A* must find the known shortest route");
    grid.set_blocked({5, 6}, true);
    require(forge2d::find_path(grid, {1, 1}, {10, 8}).empty(),
            "sealed goal region must be unreachable");
    require_throws([] { static_cast<void>(forge2d::Grid{0, 1}); },
                   "invalid grid dimensions must fail");
}

void test_scene_and_replay_round_trip() {
    const auto scene = sample_scene();
    require(forge2d::parse_scene(forge2d::serialize_scene(scene)) == scene,
            "scene serialization must round-trip exactly");
    const forge2d::Replay replay{42U, 60U, {{1, 0, false}, {0, -1, true}, {-1, 1, false}}};
    require(forge2d::parse_replay(forge2d::serialize_replay(replay)) == replay,
            "replay serialization must round-trip exactly");
    require_throws([] { static_cast<void>(forge2d::parse_scene("invalid")); },
                   "unknown scene header must fail");
    require_throws(
        [] {
            static_cast<void>(forge2d::parse_replay(
                "FORGE2D_REPLAY 1\nseed 1\ntick_rate 60\nframes 1\ninput 2 0 0\n"));
        },
        "out-of-range replay input must fail");
}

void test_world_replay_and_render_are_deterministic() {
    const auto scene = sample_scene();
    forge2d::Replay replay{9U, 60U, {}};
    for (std::size_t tick = 0; tick < 300U; ++tick) {
        replay.frames.push_back({static_cast<std::int8_t>((tick / 50U) % 2U == 0U ? 1 : -1),
                                 static_cast<std::int8_t>((tick / 75U) % 2U == 0U ? 1 : -1),
                                 false});
    }
    auto first = forge2d::World::from_scene(scene);
    auto second = forge2d::World::from_scene(scene);
    require_throws(
        [] {
            forge2d::World bounded{{0, 0}, {1'000, 1'000}};
            static_cast<void>(bounded.spawn({{100, 100}}, {{0, 0}, {200, 200}}, {}, false));
        },
        "entities must fit entirely within world bounds");
    for (const auto frame : replay.frames) {
        first.step(frame);
        second.step(frame);
    }
    require(first.state_hash() == second.state_hash(),
            "same scene and replay must produce identical state hashes");
    require(first.state_hash() == forge2d::run_replay(forge2d::World::from_scene(scene), replay),
            "replay helper must match direct simulation");
    forge2d::Image first_image{320U, 180U};
    forge2d::Image second_image{320U, 180U};
    forge2d::render_world(first_image, first);
    forge2d::render_world(second_image, second);
    require(first_image.pixel_hash() == second_image.pixel_hash() && first_image.pixel_hash() != 0U,
            "deterministic states must render identical non-empty frames");
    const auto player = first.player();
    require(player.has_value() && first.destroy(*player), "player must be destroyable once");
    require(!first.destroy(*player) && !first.valid(*player), "stale entity destruction must fail");
}

void test_profiler_summary_is_bounded() {
    forge2d::FrameProfiler profiler{3U};
    profiler.record(std::chrono::microseconds{10});
    profiler.record(std::chrono::microseconds{20});
    profiler.record(std::chrono::microseconds{30});
    profiler.record(std::chrono::microseconds{40});
    const auto summary = profiler.summary();
    require(summary.sample_count == 3U && summary.maximum_microseconds == 40.0,
            "profiler must retain only the bounded rolling window");
    require(summary.mean_microseconds > 29.0 && summary.mean_microseconds < 31.0,
            "profiler mean must summarize retained samples");
}

} // namespace

int main() { // NOLINT(bugprone-exception-escape)
    const std::vector<Test> tests{
        {"math and fixed step", test_math_and_fixed_step},
        {"entity pool and sparse-set properties", test_entity_pool_and_sparse_set_properties},
        {"C arena alignment and overflow", test_c_arena_alignment_and_overflow},
        {"spatial hash reference equivalence", test_spatial_hash_matches_brute_force},
        {"A-star shortest path", test_astar_returns_valid_shortest_path},
        {"scene and replay round trip", test_scene_and_replay_round_trip},
        {"world replay and render determinism", test_world_replay_and_render_are_deterministic},
        {"profiler bounded summary", test_profiler_summary_is_bounded},
    };
    std::size_t failures{};
    for (const auto& [name, test] : tests) {
        try {
            test();
            std::cout << "PASS: " << name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "FAIL: " << name << ": " << error.what() << '\n';
        }
    }
    std::cout << "RESULT: " << (tests.size() - failures) << '/' << tests.size()
              << " test groups passed\n";
    return failures == 0U ? 0 : 1;
}
