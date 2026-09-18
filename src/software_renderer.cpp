/**
 * File: software_renderer.cpp
 * Purpose: Render world snapshots into deterministic RGBA pixels and portable PPM evidence.
 * Symbols and line locations: see docs/code-index.md; pixel_hash supports golden-image tests.
 */
#include "forge2d/renderer.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <limits>
#include <stdexcept>

namespace forge2d {
namespace {

constexpr std::uint64_t fnv_offset = 14'695'981'039'346'656'037ULL;
constexpr std::uint64_t fnv_prime = 1'099'511'628'211ULL;

[[nodiscard]] std::uint8_t red(std::uint32_t rgba) noexcept {
    return static_cast<std::uint8_t>((rgba >> 24U) & 0xFFU);
}
[[nodiscard]] std::uint8_t green(std::uint32_t rgba) noexcept {
    return static_cast<std::uint8_t>((rgba >> 16U) & 0xFFU);
}
[[nodiscard]] std::uint8_t blue(std::uint32_t rgba) noexcept {
    return static_cast<std::uint8_t>((rgba >> 8U) & 0xFFU);
}

[[nodiscard]] std::int32_t project_coordinate(std::int64_t coordinate, std::int32_t world_extent,
                                              std::uint32_t pixel_extent) noexcept {
    const auto projected = coordinate * static_cast<std::int64_t>(pixel_extent) / world_extent;
    return static_cast<std::int32_t>(
        std::clamp(projected, static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min()),
                   static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max())));
}

} // namespace

Image::Image(std::uint32_t width, std::uint32_t height) : width_(width), height_(height) {
    if (width_ == 0U || height_ == 0U || width_ > 8'192U || height_ > 8'192U) {
        throw std::invalid_argument("image dimensions must be in the range 1..8192");
    }
    pixels_.resize(static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_));
}

void Image::clear(std::uint32_t rgba) noexcept {
    std::fill(pixels_.begin(), pixels_.end(), rgba);
}

void Image::fill_rect(std::int32_t left, std::int32_t top, std::int32_t right, std::int32_t bottom,
                      std::uint32_t rgba) noexcept {
    const auto clipped_left = std::clamp(left, 0, static_cast<std::int32_t>(width_));
    const auto clipped_right = std::clamp(right, 0, static_cast<std::int32_t>(width_));
    const auto clipped_top = std::clamp(top, 0, static_cast<std::int32_t>(height_));
    const auto clipped_bottom = std::clamp(bottom, 0, static_cast<std::int32_t>(height_));
    for (auto y = clipped_top; y < clipped_bottom; ++y) {
        for (auto x = clipped_left; x < clipped_right; ++x) {
            const auto index = static_cast<std::size_t>(y) * width_ + static_cast<std::size_t>(x);
            pixels_[index] = rgba;
        }
    }
}

void Image::write_ppm(const std::filesystem::path& path) const {
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }
    std::ofstream output{path, std::ios::binary};
    if (!output) {
        throw std::runtime_error("unable to open screenshot output");
    }
    output << "P6\n" << width_ << ' ' << height_ << "\n255\n";
    for (const auto pixel : pixels_) {
        const std::array channels{red(pixel), green(pixel), blue(pixel)};
        output.write(reinterpret_cast<const char*>(channels.data()),
                     static_cast<std::streamsize>(channels.size()));
    }
    if (!output) {
        throw std::runtime_error("unable to write screenshot output");
    }
}

std::uint64_t Image::pixel_hash() const noexcept {
    auto hash = fnv_offset;
    for (const auto pixel : pixels_) {
        for (std::size_t byte = 0; byte < 4U; ++byte) {
            hash ^= (pixel >> (byte * 8U)) & 0xFFU;
            hash *= fnv_prime;
        }
    }
    return hash;
}

void render_world(Image& image, const World& world, Vec2i world_size) {
    if (world_size.x <= 0 || world_size.y <= 0) {
        throw std::invalid_argument("render world size must be positive");
    }
    image.clear(0x101827FFU);
    for (const auto& entity : world.snapshot()) {
        const auto left = project_coordinate(
            static_cast<std::int64_t>(entity.transform.position.x) - entity.body.half_extent.x,
            world_size.x, image.width());
        const auto right = project_coordinate(
            static_cast<std::int64_t>(entity.transform.position.x) + entity.body.half_extent.x,
            world_size.x, image.width());
        const auto top = project_coordinate(static_cast<std::int64_t>(entity.transform.position.y) -
                                                entity.body.half_extent.y,
                                            world_size.y, image.height());
        const auto bottom = project_coordinate(
            static_cast<std::int64_t>(entity.transform.position.y) + entity.body.half_extent.y,
            world_size.y, image.height());
        image.fill_rect(left, top, right, bottom, entity.sprite.rgba);
    }
}

} // namespace forge2d
