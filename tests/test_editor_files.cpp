/**
 * File: test_editor_files.cpp
 * Purpose: Exercise real Windows handles, bounded UTF-8 reads, atomic replacement and hardlink
 * refusal. Functions/variables and locations: docs/code-index.md. Only a new temporary test
 * directory is used.
 */
#include "../apps/editor_files.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool value) {
    if (!value) {
        throw std::runtime_error("native file invariant failed");
    }
}

template <typename Operation> void refused(Operation operation) {
    bool rejected{};
    try {
        operation();
    } catch (const std::exception&) {
        rejected = true;
    }
    require(rejected);
}
} // namespace

int main() {
    using namespace forge2d::desktop;
    wchar_t temporary[MAX_PATH]{};
    wchar_t name[MAX_PATH]{};
    if (GetTempPathW(MAX_PATH, temporary) == 0U ||
        GetTempFileNameW(temporary, L"f2d", 0, name) == 0U) {
        return 2;
    }
    DeleteFileW(name);
    const std::filesystem::path root{name};
    try {
        ensure_profile(root);
        const auto file = root / L"scene.scene";
        atomic_text(file, "first\n");
        require(read_text(file) == "first\n");
        atomic_text(file, "replacement\n");
        require(read_text(file) == "replacement\n");
        const auto linked = root / L"linked.scene";
        require(CreateHardLinkW(linked.c_str(), file.c_str(), nullptr) != FALSE);
        refused([&] { (void)read_text(file); });
        refused([&] { atomic_text(file, "must not replace"); });
        require(DeleteFileW(linked.c_str()) != FALSE);
        require(read_text(file) == "replacement\n");
        refused([&] { (void)read_text(root); });
        refused([&] { (void)read_text(root / L"missing.scene"); });
        {
            std::ofstream oversized{root / L"large.scene", std::ios::binary};
            oversized << std::string(1'048'577U, 'x');
        }
        refused([&] { (void)read_text(root / L"large.scene"); });
        refused([&] { atomic_text(file, std::string(1'048'577U, 'x')); });
        require(read_text(file) == "replacement\n");
        atomic_text(file, std::string{"\xff\xfe", 2});
        refused([&] { (void)read_text(file); });
        const auto unicode = std::string{"unit \xe2\x86\x92 value"};
        require(utf8(wide(unicode)) == unicode);
        atomic_text(file, "final\n");
        require(read_text(file) == "final\n");
        for (const auto& member : std::filesystem::directory_iterator(root)) {
            require(member.path().filename().wstring().find(L".tmp-") == std::wstring::npos);
        }
        std::filesystem::remove_all(root);
        std::cout
            << "Native file boundary: actual save/replace/read/link/UTF-8/budget cases passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        std::filesystem::remove_all(root);
        return 1;
    }
}
