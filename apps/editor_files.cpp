/**
 * File: editor_files.cpp
 * Purpose: Keep native project files outside the executable, bound actual reads and atomically
 * save. Functions/variables: see docs/code-index.md. Same-OS-user access is trusted; no encryption
 * is claimed.
 */
#include "editor_files.hpp"

#include "forge2d/editor.hpp"

#include <bcrypt.h>
#include <shlobj.h>

#include <array>
#include <limits>
#include <stdexcept>
#include <vector>

namespace forge2d::desktop {
namespace {
[[noreturn]] void file_error() {
    throw std::runtime_error("File unavailable, unsafe, locked, too large, or not writable.");
}

void regular(HANDLE handle) {
    BY_HANDLE_FILE_INFORMATION info{};
    if (handle == INVALID_HANDLE_VALUE || GetFileType(handle) != FILE_TYPE_DISK ||
        !GetFileInformationByHandle(handle, &info) || info.nNumberOfLinks != 1U ||
        (info.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) != 0U) {
        file_error();
    }
}

[[nodiscard]] std::wstring random_suffix() {
    std::array<unsigned char, 16> bytes{};
    if (BCryptGenRandom(nullptr, bytes.data(), static_cast<ULONG>(bytes.size()),
                        BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) {
        file_error();
    }
    constexpr wchar_t digits[] = L"0123456789abcdef";
    std::wstring result;
    for (const auto byte : bytes) {
        result += digits[byte >> 4U];
        result += digits[byte & 15U];
    }
    return result;
}
} // namespace

Handle::~Handle() {
    if (value_ != INVALID_HANDLE_VALUE && value_ != nullptr) {
        CloseHandle(value_);
    }
}

std::string read_text(const std::filesystem::path& path) {
    const Handle file{CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                                  OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr)};
    regular(file.get());
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(file.get(), &size) || size.QuadPart < 0 ||
        size.QuadPart > static_cast<LONGLONG>(EditorSession::maximum_input_bytes)) {
        file_error();
    }
    std::string text(static_cast<std::size_t>(size.QuadPart), '\0');
    DWORD received{};
    if (!ReadFile(file.get(), text.data(), static_cast<DWORD>(text.size()), &received, nullptr) ||
        received != text.size()) {
        file_error();
    }
    char extra{};
    if (!ReadFile(file.get(), &extra, 1, &received, nullptr) || received != 0U) {
        file_error();
    }
    (void)wide(text); // Reject invalid UTF-8 before it reaches the native text control.
    return text;
}

void atomic_text(const std::filesystem::path& path, std::string_view text) {
    if (text.size() > EditorSession::maximum_input_bytes) {
        file_error();
    }
    const auto attributes = GetFileAttributesW(path.c_str());
    if (attributes != INVALID_FILE_ATTRIBUTES) {
        const Handle existing{CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                                          OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr)};
        regular(existing.get());
    }
    const auto staging = std::filesystem::path{path.wstring() + L".tmp-" + random_suffix()};
    try {
        {
            const Handle file{CreateFileW(staging.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                                          FILE_ATTRIBUTE_NORMAL, nullptr)};
            regular(file.get());
            DWORD written{};
            if (!WriteFile(file.get(), text.data(), static_cast<DWORD>(text.size()), &written,
                           nullptr) ||
                written != text.size() || !FlushFileBuffers(file.get())) {
                file_error();
            }
        }
        if (!MoveFileExW(staging.c_str(), path.c_str(),
                         MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            file_error();
        }
    } catch (...) {
        DeleteFileW(staging.c_str());
        throw;
    }
}

std::filesystem::path executable_directory() {
    std::vector<wchar_t> path(32'768U);
    const auto length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if (length == 0U || length >= path.size()) {
        file_error();
    }
    return std::filesystem::path{std::wstring{path.data(), length}}.parent_path();
}

std::filesystem::path default_profile() {
    PWSTR result{};
    if (SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, nullptr, &result) != S_OK) {
        file_error();
    }
    const std::filesystem::path path{result};
    CoTaskMemFree(result);
    return path / L"Forge2DEditor";
}

void ensure_profile(const std::filesystem::path& path) {
    std::filesystem::create_directories(path);
    const auto attributes = GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES ||
        (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0U) {
        file_error();
    }
}

std::wstring wide(std::string_view text) {
    if (text.empty()) {
        return {};
    }
    if (text.size() > EditorSession::maximum_input_bytes) {
        file_error();
    }
    const auto length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                                            static_cast<int>(text.size()), nullptr, 0);
    if (length == 0) {
        file_error();
    }
    std::wstring result(static_cast<std::size_t>(length), L'\0');
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                            static_cast<int>(text.size()), result.data(), length) != length) {
        file_error();
    }
    return result;
}

std::string utf8(std::wstring_view text) {
    if (text.empty()) {
        return {};
    }
    if (text.size() > EditorSession::maximum_input_bytes) {
        file_error();
    }
    const auto length =
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(),
                            static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (length == 0 || length > static_cast<int>(EditorSession::maximum_input_bytes)) {
        file_error();
    }
    std::string result(static_cast<std::size_t>(length), '\0');
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(),
                            static_cast<int>(text.size()), result.data(), length, nullptr,
                            nullptr) != length) {
        file_error();
    }
    return result;
}
} // namespace forge2d::desktop
