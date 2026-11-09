/**
 * File: forge2d_arena.cpp
 * Purpose: Run the deterministic headless arena demo and emit replay/screenshot evidence.
 * Symbols and line locations: see docs/code-index.md; main is a script-friendly recruiter demo.
 */
#include "forge2d/pathfinding.hpp"
#include "forge2d/renderer.hpp"
#include "forge2d/replay.hpp"
#include "forge2d/scene.hpp"
#include "forge2d/world.hpp"

#include "arena.h"

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

[[nodiscard]] std::string read_text(const std::filesystem::path& path) {
    if (std::filesystem::file_size(path) > 1'048'576U) {
        throw std::runtime_error("scene exceeds one-megabyte demo limit");
    }
    std::ifstream input{path};
    if (!input) {
        throw std::runtime_error("unable to open scene: " + path.string());
    }
    return {std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
}

[[nodiscard]] forge2d::Replay build_autopilot(std::size_t tick_count) {
    forge2d::Grid grid{20, 20};
    for (std::int32_t y = 3; y < 17; ++y) {
        if (y != 9) {
            grid.set_blocked({10, y}, true);
        }
    }
    const auto path = forge2d::find_path(grid, {1, 1}, {18, 18});
    if (path.empty()) {
        throw std::runtime_error("autopilot path is unexpectedly unreachable");
    }
    forge2d::Replay replay{0xF0A6E2DULL, 60U, {}};
    replay.frames.reserve(tick_count);
    for (std::size_t tick = 0; tick < tick_count; ++tick) {
        const auto segment = path[(tick / 4U) % path.size()];
        const auto next = path[((tick / 4U) + 1U) % path.size()];
        replay.frames.push_back(
            {static_cast<std::int8_t>((next.x > segment.x) - (next.x < segment.x)),
             static_cast<std::int8_t>((next.y > segment.y) - (next.y < segment.y)), false});
    }
    return replay;
}

} // namespace

int main(int argc, char** argv) { // NOLINT(bugprone-exception-escape)
    try {
        const auto scene_path =
            argc > 1 ? std::filesystem::path{argv[1]} : std::filesystem::path{"assets/arena.scene"};
        const auto output_directory =
            argc > 2 ? std::filesystem::path{argv[2]} : std::filesystem::path{"out/demo"};
        const auto scene = forge2d::parse_scene(read_text(scene_path));
        auto world = forge2d::World::from_scene(scene);
        const auto initial_pairs = world.collision_pairs();
        const auto replay = build_autopilot(360U);
        for (const auto frame : replay.frames) {
            world.step(frame);
        }
        const auto final_pairs = world.collision_pairs();
        forge2d::Image image{640U, 360U};
        forge2d::render_world(image, world);
        std::filesystem::create_directories(output_directory);
        std::ofstream contacts{output_directory / "collision-pairs.csv"};
        contacts.exceptions(std::ios::badbit | std::ios::failbit);
        contacts << "snapshot,tick,first_index,first_generation,second_index,second_generation,"
                    "first_category,first_mask,second_category,second_mask\n";
        std::map<forge2d::Entity, forge2d::CollisionFilter> filters;
        for (const auto& entity : world.snapshot())
            filters.emplace(entity.entity, entity.body.filter);
        const auto write_pairs = [&](const char* snapshot, std::uint64_t tick, const auto& pairs) {
            for (const auto& pair : pairs) {
                const auto& first = filters.at(pair.first);
                const auto& second = filters.at(pair.second);
                contacts << snapshot << ',' << tick << ',' << pair.first.index << ','
                         << pair.first.generation << ',' << pair.second.index << ','
                         << pair.second.generation << ',' << first.category << ',' << first.mask
                         << ',' << second.category << ',' << second.mask << '\n';
            }
        };
        write_pairs("initial", 0U, initial_pairs);
        write_pairs("final", world.tick(), final_pairs);
        std::ofstream canonical{output_directory / "scene-v2.scene"};
        canonical.exceptions(std::ios::badbit | std::ios::failbit);
        canonical << forge2d::serialize_scene(scene);
        image.write_ppm(output_directory / "arena-final.ppm");
        std::ofstream{output_directory / "autopilot.replay"} << forge2d::serialize_replay(replay);

        std::array<std::byte, 4'096> scratch{};
        forge2d_arena arena{};
        forge2d_arena_init(&arena, scratch.data(), scratch.size());
        static_cast<void>(forge2d_arena_allocate(&arena, 512U, 64U));
        std::cout << "{\n"
                  << "  \"engine_version\": \"" << FORGE2D_VERSION << "\",\n"
                  << "  \"ticks\": " << world.tick() << ",\n"
                  << "  \"entities\": " << world.entity_count() << ",\n"
                  << "  \"collisions\": " << final_pairs.size() << ",\n"
                  << "  \"initial_filtered_pairs\": " << initial_pairs.size() << ",\n"
                  << "  \"canonical_scene_schema\": 2,\n"
                  << "  \"state_hash\": " << world.state_hash() << ",\n"
                  << "  \"pixel_hash\": " << image.pixel_hash() << ",\n"
                  << "  \"arena_peak_bytes\": " << arena.peak << "\n"
                  << "}\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "forge2d_arena: " << error.what() << '\n';
        return 1;
    }
}
