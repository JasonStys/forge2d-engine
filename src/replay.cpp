/**
 * File: replay.cpp
 * Purpose: Serialize and strictly parse bounded, versioned deterministic input recordings.
 * Symbols and line locations: see docs/code-index.md; parser rejects trailing or oversized data.
 */
#include "forge2d/replay.hpp"

#include <sstream>
#include <stdexcept>

namespace forge2d {
namespace {

constexpr std::size_t maximum_frames = 1'000'000U;

void require_line_end(std::istringstream& line) {
    std::string trailing;
    if (line >> trailing) {
        throw std::invalid_argument("unexpected replay field");
    }
}

} // namespace

std::string serialize_replay(const Replay& replay) {
    if (replay.tick_rate == 0U || replay.tick_rate > 1'000U ||
        replay.frames.size() > maximum_frames) {
        throw std::invalid_argument("replay bounds are invalid");
    }
    std::ostringstream output;
    output << "FORGE2D_REPLAY 1\n";
    output << "seed " << replay.seed << '\n';
    output << "tick_rate " << replay.tick_rate << '\n';
    output << "frames " << replay.frames.size() << '\n';
    for (const auto frame : replay.frames) {
        output << "input " << static_cast<int>(frame.move_x) << ' '
               << static_cast<int>(frame.move_y) << ' ' << (frame.action ? 1 : 0) << '\n';
    }
    return output.str();
}

Replay parse_replay(std::string_view text) {
    std::istringstream input{std::string{text}};
    std::string line_text;
    if (!std::getline(input, line_text) || line_text != "FORGE2D_REPLAY 1") {
        throw std::invalid_argument("unsupported replay header");
    }
    Replay replay;
    std::string key;
    if (!std::getline(input, line_text)) {
        throw std::invalid_argument("missing replay seed");
    }
    {
        std::istringstream line{line_text};
        if (!(line >> key >> replay.seed) || key != "seed") {
            throw std::invalid_argument("invalid replay seed");
        }
        require_line_end(line);
    }
    if (!std::getline(input, line_text)) {
        throw std::invalid_argument("missing replay tick rate");
    }
    {
        std::istringstream line{line_text};
        if (!(line >> key >> replay.tick_rate) || key != "tick_rate" || replay.tick_rate == 0U ||
            replay.tick_rate > 1'000U) {
            throw std::invalid_argument("invalid replay tick rate");
        }
        require_line_end(line);
    }
    std::size_t frame_count{};
    if (!std::getline(input, line_text)) {
        throw std::invalid_argument("missing replay frame count");
    }
    {
        std::istringstream line{line_text};
        if (!(line >> key >> frame_count) || key != "frames" || frame_count > maximum_frames) {
            throw std::invalid_argument("invalid replay frame count");
        }
        require_line_end(line);
    }
    replay.frames.reserve(frame_count);
    for (std::size_t index = 0; index < frame_count; ++index) {
        if (!std::getline(input, line_text)) {
            throw std::invalid_argument("replay ended before declared frame count");
        }
        int move_x{};
        int move_y{};
        int action{};
        std::istringstream line{line_text};
        if (!(line >> key >> move_x >> move_y >> action) || key != "input" || move_x < -1 ||
            move_x > 1 || move_y < -1 || move_y > 1 || (action != 0 && action != 1)) {
            throw std::invalid_argument("invalid replay input frame");
        }
        require_line_end(line);
        replay.frames.push_back(
            {static_cast<std::int8_t>(move_x), static_cast<std::int8_t>(move_y), action == 1});
    }
    while (std::getline(input, line_text)) {
        if (!line_text.empty()) {
            throw std::invalid_argument("unexpected data after replay frames");
        }
    }
    return replay;
}

} // namespace forge2d
