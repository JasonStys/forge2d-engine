/**
 * File: replay.hpp
 * Purpose: Define a versioned, bounded deterministic input replay format and parser.
 * Symbols and line locations: see docs/code-index.md; Replay records seed, tick rate, and frames.
 */
#pragma once

#include "forge2d/components.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace forge2d {

struct Replay {
    std::uint64_t seed{};
    std::uint32_t tick_rate{60U};
    std::vector<InputFrame> frames{};

    auto operator<=>(const Replay&) const = default;
};

[[nodiscard]] std::string serialize_replay(const Replay& replay);
[[nodiscard]] Replay parse_replay(std::string_view text);

} // namespace forge2d
