/**
 * File: editor.hpp
 * Purpose: Define a bounded editor/play model over the actual scene, world, renderer and replay
 * APIs. Symbols and important variable lines are indexed in docs/code-index.md; UI and disk IO stay
 * outside.
 */
#pragma once

#include "forge2d/renderer.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace forge2d {

/** Keep authored scene distinct from the running world and recorded/replayed input history. */
class EditorSession {
  public:
    static constexpr std::size_t maximum_input_bytes = 1'048'576U;
    static constexpr std::size_t maximum_entities = 1'000U;
    static constexpr std::size_t maximum_frames = 36'000U;

    explicit EditorSession(std::string_view scene_text);
    void apply_scene(std::string_view text);
    void load_replay(std::string_view text);
    void reset();
    void play();
    void pause() noexcept;
    void step(InputFrame input = {});
    void advance(InputFrame input = {});

    [[nodiscard]] bool playing() const noexcept {
        return playing_;
    }
    [[nodiscard]] bool reviewing_replay() const noexcept {
        return replay_.has_value();
    }
    [[nodiscard]] std::size_t replay_remaining() const noexcept;
    [[nodiscard]] const Scene& scene() const noexcept {
        return scene_;
    }
    [[nodiscard]] const World& world() const noexcept {
        return world_;
    }
    [[nodiscard]] const Replay& recording() const noexcept {
        return recording_;
    }
    [[nodiscard]] std::string scene_text() const;
    [[nodiscard]] std::string replay_text() const;
    [[nodiscard]] Image image(std::uint32_t width, std::uint32_t height) const;

  private:
    Scene scene_{};
    World world_{};
    Replay recording_{};
    std::optional<Replay> replay_{};
    std::size_t replay_index_{};
    bool playing_{};
};

} // namespace forge2d
