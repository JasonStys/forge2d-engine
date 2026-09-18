/**
 * File: forge2d_arena_sdl.cpp
 * Purpose: Run the interactive SDL3 arena using fixed 60 Hz simulation and interpolated frames.
 * Symbols and line locations: see docs/code-index.md; main owns wall-clock and presentation only.
 */
#include "forge2d/fixed_step.hpp"
#include "forge2d/scene.hpp"
#include "forge2d/sdl_renderer.hpp"
#include "forge2d/world.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <thread>

int main() {
    try {
        std::ifstream input{"assets/arena.scene"};
        if (!input) {
            throw std::runtime_error("run from the repository root so assets/arena.scene is found");
        }
        const std::string scene_text{std::istreambuf_iterator<char>{input},
                                     std::istreambuf_iterator<char>{}};
        auto world = forge2d::World::from_scene(forge2d::parse_scene(scene_text));
        forge2d::SdlRenderer renderer{"Forge2D Arena", 960, 540};
        forge2d::FixedStepClock clock{std::chrono::nanoseconds{1'000'000'000LL / 60LL}};
        auto previous = std::chrono::steady_clock::now();
        forge2d::InputFrame input_frame{};
        while (renderer.poll(input_frame)) {
            const auto now = std::chrono::steady_clock::now();
            for (std::size_t tick = 0; tick < clock.consume(now - previous); ++tick) {
                world.step(input_frame);
            }
            previous = now;
            renderer.draw(world);
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "forge2d_arena_sdl: " << error.what() << '\n';
        return 1;
    }
}
