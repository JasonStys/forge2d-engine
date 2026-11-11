/**
 * File: test_editor.cpp
 * Purpose: Independently verify editor scene/replay transitions, input limits and actual rendering.
 * Symbols and important variable lines are indexed in docs/code-index.md; all fixtures are
 * synthetic.
 */
#include "forge2d/editor.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
constexpr std::string_view scene_text = "FORGE2D_SCENE 2\nbounds 0 0 10000 10000\nentities 2\n"
                                        "entity player 1000 1000 0 0 100 100 16711935 10 1 1 2\n"
                                        "entity zone 1150 1000 0 0 100 100 4278190335 5 0 2 1\n";

void require(bool condition, const char* message) {
    if (!condition)
        throw std::runtime_error(message);
}
template <typename Function> void rejects(Function&& operation, const char* message) {
    try {
        operation();
    } catch (const std::exception&) {
        return;
    }
    throw std::runtime_error(message);
}

void test_authored_input_and_replay() {
    forge2d::EditorSession session{scene_text};
    require(!session.playing() && session.world().tick() == 0U, "editor must begin paused");
    require(session.world().collision_pairs().size() == 1U, "authored compatible zones overlap");
    session.advance({1, 0, false});
    require(session.world().tick() == 0U, "paused timer cannot advance the actual world");
    session.play();
    const std::array<forge2d::InputFrame, 4> inputs{
        {{1, 0, false}, {1, 0, true}, {0, 1, false}, {0, 0, false}}};
    for (const auto input : inputs)
        session.advance(input);
    session.pause();
    const auto snapshot = session.world().snapshot();
    const auto player = std::find_if(snapshot.begin(), snapshot.end(),
                                     [](const auto& entity) { return entity.player; });
    require(player != snapshot.end() && player->transform.position == forge2d::Vec2i{1280, 1140},
            "four authored player inputs must yield x1280/y1140");
    require(session.recording().frames ==
                std::vector<forge2d::InputFrame>(inputs.begin(), inputs.end()),
            "recording must contain actual applied inputs");
    const auto expected = session.world().state_hash();
    const auto replay = session.replay_text();
    session.load_replay(replay);
    require(session.reviewing_replay() && session.replay_remaining() == 4U && !session.playing(),
            "replay load must reset and pause actual world");
    session.play();
    for (std::size_t index = 0; index < inputs.size(); ++index)
        session.advance({-1, -1, true});
    require(session.world().state_hash() == expected && !session.playing(),
            "playback must ignore live input and exactly reproduce original state");
    require(session.replay_remaining() == 0U, "completed replay must have no invented frames");
    rejects([&] { session.play(); }, "completed replay must refuse resumed play");
    rejects([&] { session.step(); }, "completed replay must refuse invented manual frames");
    session.reset();
    require(!session.reviewing_replay() && session.recording().frames.empty() &&
                session.world().tick() == 0U,
            "reset restores authored scene and clears transient input history");
}

void test_apply_is_transactional() {
    forge2d::EditorSession session{scene_text};
    session.step({1, 0, false});
    const auto hash = session.world().state_hash();
    const auto canonical = session.scene_text();
    rejects([&] { session.apply_scene("bad scene"); }, "invalid text must be refused");
    rejects(
        [&] {
            session.apply_scene(std::string(forge2d::EditorSession::maximum_input_bytes + 1U, 'x'));
        },
        "oversized source must be refused");
    auto invalid = forge2d::parse_scene(scene_text);
    invalid.entities[0].transform.position = {0, 0};
    rejects([&] { session.apply_scene(forge2d::serialize_scene(invalid)); },
            "engine must reject a body that does not fit bounds");
    require(session.scene_text() == canonical && session.world().state_hash() == hash &&
                session.recording().frames.size() == 1U,
            "refused apply must preserve authored scene and running/recorded state");
    rejects([&] { session.step({2, 0, false}); }, "invalid live movement must be refused");
    require(session.world().state_hash() == hash, "invalid movement must not mutate actual world");
    auto updated = forge2d::parse_scene(scene_text);
    updated.entities[0].body.filter.mask = 0U;
    session.apply_scene(forge2d::serialize_scene(updated));
    require(session.world().collision_pairs().empty(),
            "applying edited scene must change actual collision filters");
    require(session.world().tick() == 0U && session.recording().frames.empty(),
            "accepted edit starts a new paused run");
    session.apply_scene("FORGE2D_SCENE 2\nbounds 0 0 1000 1000\nentities 0\n");
    require(session.world().entity_count() == 0U, "empty project is a valid editor boundary");
}

void test_replay_limits() {
    forge2d::EditorSession session{scene_text};
    const auto old_hash = session.world().state_hash();
    rejects([&] { session.load_replay("bad replay"); }, "malformed replay must be refused");
    rejects(
        [&] {
            session.load_replay(std::string(forge2d::EditorSession::maximum_input_bytes + 1U, 'x'));
        },
        "oversized replay must be refused");
    forge2d::Replay replay{0U, 120U, {}};
    rejects([&] { session.load_replay(forge2d::serialize_replay(replay)); },
            "native editor must not reinterpret a different tick rate");
    replay.tick_rate = 60U;
    replay.frames.resize(forge2d::EditorSession::maximum_frames + 1U);
    rejects([&] { session.load_replay(forge2d::serialize_replay(replay)); },
            "native replay cap must be checked before state mutation");
    require(session.world().state_hash() == old_hash, "refused replay must preserve project state");
    replay.frames.clear();
    session.load_replay(forge2d::serialize_replay(replay));
    rejects([&] { session.play(); }, "zero-frame replay must remain explicit and paused");
    session.reset();
    for (std::size_t index = 0; index < forge2d::EditorSession::maximum_frames; ++index)
        session.step();
    rejects([&] { session.step(); }, "recording must not silently discard input on overflow");
    rejects([&] { session.play(); }, "full recording must not restart without reset/export");
    require(session.world().tick() == forge2d::EditorSession::maximum_frames,
            "failed overflow must not advance simulation");
}

void test_entities_and_offset_rendering() {
    forge2d::EditorSession session{scene_text};
    auto scene = forge2d::parse_scene(scene_text);
    const auto prototype = scene.entities.back();
    scene.entities.clear();
    for (std::size_t index = 0; index < forge2d::EditorSession::maximum_entities + 1U; ++index) {
        auto entity = prototype;
        entity.name = "entity_" + std::to_string(index);
        scene.entities.push_back(entity);
    }
    rejects([&] { session.apply_scene(forge2d::serialize_scene(scene)); },
            "native entity budget must be enforced");
    scene.entities.pop_back();
    session.apply_scene(forge2d::serialize_scene(scene));
    require(session.world().entity_count() == forge2d::EditorSession::maximum_entities,
            "exact native entity boundary must remain accepted");
    session.apply_scene(scene_text);
    const auto origin_image = session.image(100U, 100U);
    scene = forge2d::parse_scene(scene_text);
    scene.minimum = {-5000, -5000};
    scene.maximum = {5000, 5000};
    for (auto& entity : scene.entities) {
        entity.transform.position.x -= 5000;
        entity.transform.position.y -= 5000;
    }
    session.apply_scene(forge2d::serialize_scene(scene));
    require(session.image(100U, 100U).pixel_hash() == origin_image.pixel_hash(),
            "translated world origins must preserve actual framebuffer geometry");
    rejects([&] { static_cast<void>(session.image(0U, 100U)); },
            "invalid framebuffers must remain rejected");
}
} // namespace

int main() {
    try {
        test_authored_input_and_replay();
        test_apply_is_transactional();
        test_replay_limits();
        test_entities_and_offset_rendering();
        std::cout << "Editor model: 4 independent test groups passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
