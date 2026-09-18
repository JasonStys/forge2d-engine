/**
 * File: scene.cpp
 * Purpose: Serialize and strictly parse bounded, versioned scene manifests.
 * Symbols and line locations: see docs/code-index.md; entity names and numeric bounds are checked.
 */
#include "forge2d/scene.hpp"

#include <algorithm>
#include <cctype>
#include <set>
#include <sstream>
#include <stdexcept>

namespace forge2d {
namespace {

constexpr std::size_t maximum_entities = 10'000U;
constexpr std::int32_t maximum_coordinate = 10'000'000;

[[nodiscard]] bool valid_name(const std::string& name) {
    return !name.empty() && name.size() <= 64U &&
           std::all_of(name.begin(), name.end(), [](unsigned char character) {
               return std::isalnum(character) != 0 || character == '_' || character == '-';
           });
}

void require_line_end(std::istringstream& line) {
    std::string trailing;
    if (line >> trailing) {
        throw std::invalid_argument("unexpected scene field");
    }
}

void validate_scene(const Scene& scene) {
    if (scene.minimum.x >= scene.maximum.x || scene.minimum.y >= scene.maximum.y ||
        scene.minimum.x < -maximum_coordinate || scene.minimum.y < -maximum_coordinate ||
        scene.maximum.x > maximum_coordinate || scene.maximum.y > maximum_coordinate ||
        scene.entities.size() > maximum_entities) {
        throw std::invalid_argument("scene bounds or entity count are invalid");
    }
    std::set<std::string> names;
    std::size_t players{};
    for (const auto& entity : scene.entities) {
        if (!valid_name(entity.name) || !names.insert(entity.name).second) {
            throw std::invalid_argument("scene entity names must be unique safe identifiers");
        }
        if (entity.body.half_extent.x <= 0 || entity.body.half_extent.y <= 0 ||
            entity.transform.position.x < scene.minimum.x ||
            entity.transform.position.x > scene.maximum.x ||
            entity.transform.position.y < scene.minimum.y ||
            entity.transform.position.y > scene.maximum.y) {
            throw std::invalid_argument("scene entity geometry is invalid");
        }
        players += entity.player ? 1U : 0U;
    }
    if (players > 1U) {
        throw std::invalid_argument("a scene may define at most one player");
    }
}

} // namespace

std::string serialize_scene(const Scene& scene) {
    validate_scene(scene);
    std::ostringstream output;
    output << "FORGE2D_SCENE 1\n";
    output << "bounds " << scene.minimum.x << ' ' << scene.minimum.y << ' ' << scene.maximum.x
           << ' ' << scene.maximum.y << '\n';
    output << "entities " << scene.entities.size() << '\n';
    for (const auto& entity : scene.entities) {
        output << "entity " << entity.name << ' ' << entity.transform.position.x << ' '
               << entity.transform.position.y << ' ' << entity.body.velocity.x << ' '
               << entity.body.velocity.y << ' ' << entity.body.half_extent.x << ' '
               << entity.body.half_extent.y << ' ' << entity.sprite.rgba << ' '
               << entity.sprite.layer << ' ' << (entity.player ? 1 : 0) << '\n';
    }
    return output.str();
}

Scene parse_scene(std::string_view text) {
    std::istringstream input{std::string{text}};
    std::string line_text;
    if (!std::getline(input, line_text) || line_text != "FORGE2D_SCENE 1") {
        throw std::invalid_argument("unsupported scene header");
    }
    Scene scene;
    std::string key;
    if (!std::getline(input, line_text)) {
        throw std::invalid_argument("missing scene bounds");
    }
    {
        std::istringstream line{line_text};
        if (!(line >> key >> scene.minimum.x >> scene.minimum.y >> scene.maximum.x >>
              scene.maximum.y) ||
            key != "bounds") {
            throw std::invalid_argument("invalid scene bounds");
        }
        require_line_end(line);
    }
    std::size_t entity_count{};
    if (!std::getline(input, line_text)) {
        throw std::invalid_argument("missing scene entity count");
    }
    {
        std::istringstream line{line_text};
        if (!(line >> key >> entity_count) || key != "entities" ||
            entity_count > maximum_entities) {
            throw std::invalid_argument("invalid scene entity count");
        }
        require_line_end(line);
    }
    scene.entities.reserve(entity_count);
    for (std::size_t index = 0; index < entity_count; ++index) {
        if (!std::getline(input, line_text) || line_text.size() > 1'024U) {
            throw std::invalid_argument("scene ended before declared entity count");
        }
        SceneEntity entity;
        int player{};
        std::int32_t layer{};
        std::istringstream line{line_text};
        if (!(line >> key >> entity.name >> entity.transform.position.x >>
              entity.transform.position.y >> entity.body.velocity.x >> entity.body.velocity.y >>
              entity.body.half_extent.x >> entity.body.half_extent.y >> entity.sprite.rgba >>
              layer >> player) ||
            key != "entity" || layer < -32'768 || layer > 32'767 || (player != 0 && player != 1)) {
            throw std::invalid_argument("invalid scene entity");
        }
        require_line_end(line);
        entity.sprite.layer = static_cast<std::int16_t>(layer);
        entity.player = player == 1;
        scene.entities.push_back(std::move(entity));
    }
    while (std::getline(input, line_text)) {
        if (!line_text.empty()) {
            throw std::invalid_argument("unexpected data after scene entities");
        }
    }
    validate_scene(scene);
    return scene;
}

} // namespace forge2d
