/**
 * File: scene_parser_fuzz.cpp
 * Purpose: Fuzz the untrusted scene-parser boundary and canonical round-trip behavior.
 * Symbols and line locations: see docs/code-index.md; LLVMFuzzerTestOneInput bounds each payload.
 */
#include "forge2d/scene.hpp"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <string_view>

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    if (size > 1'048'576U) {
        return 0;
    }
    const auto text = std::string_view{reinterpret_cast<const char*>(data), size};
    try {
        const auto scene = forge2d::parse_scene(text);
        const auto canonical = forge2d::serialize_scene(scene);
        static_cast<void>(forge2d::parse_scene(canonical));
    } catch (const std::exception&) {
        // Invalid untrusted manifests are expected; crashes, leaks, and UB are not.
    }
    return 0;
}
