/**
 * File: scene.hpp
 * Purpose: Define a small versioned scene manifest with strict parsing and canonical serialization.
 * Symbols and line locations: see docs/code-index.md; SceneEntity maps directly to World spawning.
 */
#pragma once

#include "forge2d/components.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace forge2d {

struct SceneEntity {
    std::string name{};
    Transform transform{};
    KinematicBody body{};
    Sprite sprite{};
    bool player{};

    auto operator<=>(const SceneEntity&) const = default;
};

struct Scene {
    Vec2i minimum{};
    Vec2i maximum{10'000, 10'000};
    std::vector<SceneEntity> entities{};

    auto operator<=>(const Scene&) const = default;
};

[[nodiscard]] std::string serialize_scene(const Scene& scene);
[[nodiscard]] Scene parse_scene(std::string_view text);

} // namespace forge2d
