/**
 * File: renderer.hpp
 * Purpose: Define a dependency-free software framebuffer and world rendering contract.
 * Symbols and line locations: see docs/code-index.md; Image writes portable PPM screenshots.
 */
#pragma once

#include "forge2d/world.hpp"

#include <cstdint>
#include <filesystem>
#include <vector>

namespace forge2d {

class Image {
  public:
    Image(std::uint32_t width, std::uint32_t height);

    void clear(std::uint32_t rgba) noexcept;
    void fill_rect(std::int32_t left, std::int32_t top, std::int32_t right, std::int32_t bottom,
                   std::uint32_t rgba) noexcept;
    void write_ppm(const std::filesystem::path& path) const;
    [[nodiscard]] std::uint64_t pixel_hash() const noexcept;
    [[nodiscard]] std::uint32_t width() const noexcept {
        return width_;
    }
    [[nodiscard]] std::uint32_t height() const noexcept {
        return height_;
    }
    [[nodiscard]] const std::vector<std::uint32_t>& pixels() const noexcept {
        return pixels_;
    }

  private:
    std::uint32_t width_{};
    std::uint32_t height_{};
    std::vector<std::uint32_t> pixels_{};
};

void render_world(Image& image, const World& world, Vec2i world_size = {10'000, 10'000});

} // namespace forge2d
