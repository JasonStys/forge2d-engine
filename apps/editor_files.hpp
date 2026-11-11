/**
 * File: editor_files.hpp
 * Purpose: Declare Windows descriptor-bound scene reads, atomic saves and exclusive profile
 * ownership. Symbols, functions and variable locations are listed in docs/code-index.md.
 */
#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <filesystem>
#include <string>
#include <string_view>

namespace forge2d::desktop {

/** An owned OS handle is never copied or leaked to a child process. */
class Handle {
  public:
    explicit Handle(HANDLE value = INVALID_HANDLE_VALUE) noexcept : value_(value) {}
    ~Handle();
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    [[nodiscard]] HANDLE get() const noexcept {
        return value_;
    }

  private:
    HANDLE value_;
};

[[nodiscard]] std::string read_text(const std::filesystem::path& path);
void atomic_text(const std::filesystem::path& path, std::string_view text);
[[nodiscard]] std::filesystem::path executable_directory();
[[nodiscard]] std::filesystem::path default_profile();
void ensure_profile(const std::filesystem::path& path);
[[nodiscard]] std::wstring wide(std::string_view text);
[[nodiscard]] std::string utf8(std::wstring_view text);

} // namespace forge2d::desktop
