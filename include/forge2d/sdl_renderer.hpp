/**
 * File: sdl_renderer.hpp
 * Purpose: Declare the optional SDL3 window, input, and accelerated 2D rendering adapter.
 * Symbols and line locations: see docs/code-index.md; SDL types stay outside the engine core.
 */
#pragma once

#include "forge2d/components.hpp"
#include "forge2d/world.hpp"

#include <cstdint>
#include <string>

struct SDL_Renderer;
struct SDL_Window;

namespace forge2d {

class SdlRenderer {
  public:
    SdlRenderer(std::string title, std::int32_t width, std::int32_t height);
    ~SdlRenderer();

    SdlRenderer(const SdlRenderer&) = delete;
    SdlRenderer& operator=(const SdlRenderer&) = delete;
    SdlRenderer(SdlRenderer&&) = delete;
    SdlRenderer& operator=(SdlRenderer&&) = delete;

    [[nodiscard]] bool poll(InputFrame& input) const;
    void draw(const World& world) const;

  private:
    SDL_Window* window_{};
    SDL_Renderer* renderer_{};
    std::int32_t width_{};
    std::int32_t height_{};
};

} // namespace forge2d
