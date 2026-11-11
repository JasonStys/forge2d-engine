/**
 * File: editor.cpp
 * Purpose: Apply scene changes transactionally and run bounded real engine/replay workflows.
 * Symbols and important variable lines are indexed in docs/code-index.md; no platform IO occurs
 * here.
 */
#include "forge2d/editor.hpp"

#include <stdexcept>
#include <utility>

namespace forge2d {

EditorSession::EditorSession(std::string_view scene_text) {
    apply_scene(scene_text);
}

void EditorSession::apply_scene(std::string_view text) {
    if (text.size() > maximum_input_bytes) {
        throw std::invalid_argument("Editor scene input exceeds 1 MiB");
    }
    auto next_scene = parse_scene(text);
    if (next_scene.entities.size() > maximum_entities) {
        throw std::invalid_argument("Editor scenes are limited to 1000 entities");
    }
    auto next_world = World::from_scene(next_scene);
    Replay next_recording;
    next_recording.frames.reserve(maximum_frames);
    scene_ = std::move(next_scene);
    world_ = std::move(next_world);
    recording_ = std::move(next_recording);
    replay_.reset();
    replay_index_ = 0U;
    playing_ = false;
}

void EditorSession::reset() {
    auto next_world = World::from_scene(scene_);
    world_ = std::move(next_world);
    recording_.frames.clear();
    replay_.reset();
    replay_index_ = 0U;
    playing_ = false;
}

void EditorSession::load_replay(std::string_view text) {
    if (text.size() > maximum_input_bytes) {
        throw std::invalid_argument("Editor replay input exceeds 1 MiB");
    }
    auto replay = parse_replay(text);
    if (replay.tick_rate != 60U || replay.frames.size() > maximum_frames) {
        throw std::invalid_argument("Editor replay requires 60 Hz and at most 36000 frames");
    }
    auto next_world = World::from_scene(scene_);
    Replay next_recording{replay.seed, replay.tick_rate, {}};
    next_recording.frames.reserve(maximum_frames);
    world_ = std::move(next_world);
    recording_ = std::move(next_recording);
    replay_ = std::move(replay);
    replay_index_ = 0U;
    playing_ = false;
}

void EditorSession::play() {
    if (recording_.frames.size() >= maximum_frames || (replay_ && replay_remaining() == 0U)) {
        throw std::invalid_argument("Recording/replay is complete; reset or load a new input");
    }
    playing_ = true;
}

void EditorSession::pause() noexcept {
    playing_ = false;
}

std::size_t EditorSession::replay_remaining() const noexcept {
    return replay_ ? replay_->frames.size() - replay_index_ : 0U;
}

void EditorSession::step(InputFrame input) {
    if (recording_.frames.size() >= maximum_frames) {
        playing_ = false;
        throw std::invalid_argument("Recording limit reached; export before resetting");
    }
    if (replay_) {
        if (replay_remaining() == 0U) {
            playing_ = false;
            throw std::invalid_argument("Replay is complete");
        }
        input = replay_->frames[replay_index_];
    }
    if (input.move_x < -1 || input.move_x > 1 || input.move_y < -1 || input.move_y > 1) {
        throw std::invalid_argument("Movement inputs must be in -1..1");
    }
    world_.step(input);
    recording_.frames.push_back(input);
    if (replay_) {
        ++replay_index_;
        if (replay_remaining() == 0U)
            playing_ = false;
    }
}

void EditorSession::advance(InputFrame input) {
    if (playing_)
        step(input);
}

std::string EditorSession::scene_text() const {
    return serialize_scene(scene_);
}
std::string EditorSession::replay_text() const {
    return serialize_replay(recording_);
}

Image EditorSession::image(std::uint32_t width, std::uint32_t height) const {
    Image image{width, height};
    const Vec2i extent{scene_.maximum.x - scene_.minimum.x, scene_.maximum.y - scene_.minimum.y};
    render_world(image, world_, extent, scene_.minimum);
    return image;
}

} // namespace forge2d
