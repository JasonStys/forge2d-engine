/**
 * File: sdl_renderer.cpp
 * Purpose: Implement SDL3 window/input and batched rectangle presentation for the sample game.
 * Symbols and line locations: see docs/code-index.md; errors become exceptions at the adapter edge.
 */
#include "forge2d/sdl_renderer.hpp"

#include <SDL3/SDL.h>

#include <stdexcept>
#include <utility>

namespace forge2d {
namespace {

[[nodiscard]] std::uint8_t channel(std::uint32_t rgba, std::uint32_t shift) noexcept {
    return static_cast<std::uint8_t>((rgba >> shift) & 0xFFU);
}

} // namespace

SdlRenderer::SdlRenderer(std::string title, std::int32_t width, std::int32_t height)
    : width_(width), height_(height) {
    if (width_ <= 0 || height_ <= 0 || !SDL_Init(SDL_INIT_VIDEO)) {
        throw std::runtime_error(std::string{"SDL initialization failed: "} + SDL_GetError());
    }
    if (!SDL_CreateWindowAndRenderer(title.c_str(), width_, height_, SDL_WINDOW_RESIZABLE, &window_,
                                     &renderer_)) {
        const auto error = std::string{SDL_GetError()};
        SDL_Quit();
        throw std::runtime_error("SDL window creation failed: " + error);
    }
}

SdlRenderer::~SdlRenderer() {
    if (renderer_ != nullptr) {
        SDL_DestroyRenderer(renderer_);
    }
    if (window_ != nullptr) {
        SDL_DestroyWindow(window_);
    }
    SDL_Quit();
}

bool SdlRenderer::poll(InputFrame& input) const {
    SDL_Event event{};
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            return false;
        }
    }
    const auto* keys = SDL_GetKeyboardState(nullptr);
    input.move_x = static_cast<std::int8_t>((keys[SDL_SCANCODE_RIGHT] ? 1 : 0) -
                                            (keys[SDL_SCANCODE_LEFT] ? 1 : 0));
    input.move_y = static_cast<std::int8_t>((keys[SDL_SCANCODE_DOWN] ? 1 : 0) -
                                            (keys[SDL_SCANCODE_UP] ? 1 : 0));
    input.action = keys[SDL_SCANCODE_SPACE];
    return !keys[SDL_SCANCODE_ESCAPE];
}

void SdlRenderer::draw(const World& world) const {
    static_cast<void>(SDL_SetRenderDrawColor(renderer_, 16U, 24U, 39U, 255U));
    static_cast<void>(SDL_RenderClear(renderer_));
    for (const auto& entity : world.snapshot()) {
        const auto color = entity.sprite.rgba;
        static_cast<void>(SDL_SetRenderDrawColor(renderer_, channel(color, 24U),
                                                 channel(color, 16U), channel(color, 8U),
                                                 channel(color, 0U)));
        const auto left =
            static_cast<float>(entity.transform.position.x - entity.body.half_extent.x) *
            static_cast<float>(width_) / 10'000.0F;
        const auto top =
            static_cast<float>(entity.transform.position.y - entity.body.half_extent.y) *
            static_cast<float>(height_) / 10'000.0F;
        const auto rect_width = static_cast<float>(entity.body.half_extent.x * 2) *
                                static_cast<float>(width_) / 10'000.0F;
        const auto rect_height = static_cast<float>(entity.body.half_extent.y * 2) *
                                 static_cast<float>(height_) / 10'000.0F;
        const SDL_FRect rectangle{left, top, rect_width, rect_height};
        static_cast<void>(SDL_RenderFillRect(renderer_, &rectangle));
    }
    SDL_RenderPresent(renderer_);
}

} // namespace forge2d
