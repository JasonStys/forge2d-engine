/**
 * File: forge2d_editor.cpp
 * Purpose: Provide the actual native scene editor, fixed-tick play/replay, durable saves and
 * evidence. Functions/classes/important variables: docs/code-index.md. Keyboard input belongs only
 * to this window's focused canvas; no global input hooks, network, accounts, plugins or external
 * processes.
 */
#include "editor_files.hpp"

#include "forge2d/editor.hpp"
#include "forge2d/fixed_step.hpp"

// GDI+ requires the COM IStream/PROPID declarations before its own SDK headers.
#include <objidl.h>

#include <commdlg.h>
#include <gdiplus.h>
#include <shellapi.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace forge2d::desktop {
namespace {
constexpr int apply_id = 101;
constexpr int play_id = 102;
constexpr int step_id = 103;
constexpr int reset_id = 104;
constexpr int save_id = 105;
constexpr int open_id = 106;
constexpr int replay_id = 107;
constexpr int export_id = 108;
constexpr int arena_id = 109;
constexpr int layers_id = 110;
constexpr int help_id = 111;
constexpr int quit_id = 112;
constexpr int text_id = 120;
constexpr UINT_PTR frame_timer = 1U;
constexpr UINT_PTR acceptance_timer = 2U;
constexpr wchar_t help_text[] =
    L"Forge2D Editor 1.1.0 | Created by Jason Stys\n\n"
    L"1. Open a scene or choose a prepared example. Edit the strict scene text; Apply validates "
    L"all "
    L"fields before replacing the running world. Invalid edits preserve the applied scene.\n"
    L"2. Click the canvas. WASD/arrow keys control the player, Space records action. Play advances "
    L"60 fixed ticks/second; Step advances once. Reset returns to the authored scene.\n"
    L"3. Save project stores the applied scene in your local profile. Unapplied edits are not "
    L"saved. "
    L"Import replay plays recorded inputs against the currently applied scene. Export evidence "
    L"writes that scene, its replay, a PPM image and numeric snapshot together.\n\n"
    L"Scene row v2: entity NAME X Y VX VY HALF_W HALF_H RGBA LAYER PLAYER CATEGORY MASK. "
    L"Integers only; category is one positive bit. Mask zero disables matching. Bounds must "
    L"contain "
    L"every body. Native limits: 1 MiB input, 1,000 entities, 36,000 frames at 60 Hz.\n\n"
    L"AABB masks report candidate overlaps; they do not provide physical collision response. "
    L"Unrecorded real-time scheduling drops excess ticks instead of accumulating lag. Exported "
    L"replays require the matching scene. Local files are plaintext under your OS account; do not "
    L"put secrets in scene names. Closing releases only this application's profile lock.\n\n"
    L"Contact: github.com/JasonStys | Windows portable app; unsigned, no installer required.";

/** Retrieve only this editor's text; size validation precedes allocation and UTF-8 conversion. */
[[nodiscard]] std::string editor_text(HWND control) {
    const auto count = GetWindowTextLengthW(control);
    if (count < 0 || count > static_cast<int>(EditorSession::maximum_input_bytes)) {
        throw std::invalid_argument("Scene text exceeds the 1 MiB native editor limit.");
    }
    std::wstring text(static_cast<std::size_t>(count) + 1U, L'\0');
    const auto received = GetWindowTextW(control, text.data(), count + 1);
    text.resize(static_cast<std::size_t>(received));
    auto encoded = utf8(text);
    std::string portable;
    portable.reserve(encoded.size());
    for (std::size_t index = 0; index < encoded.size(); ++index) {
        if (encoded[index] == '\r' && index + 1U < encoded.size() && encoded[index + 1U] == '\n') {
            continue;
        }
        portable += encoded[index];
    }
    return portable;
}

[[nodiscard]] std::filesystem::path choose_file(HWND owner, bool save, const wchar_t* filter) {
    std::array<wchar_t, 32'768> path{};
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = owner;
    dialog.lpstrFilter = filter;
    dialog.lpstrFile = path.data();
    dialog.nMaxFile = static_cast<DWORD>(path.size());
    dialog.Flags =
        OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
    dialog.lpstrDefExt = L"scene";
    if ((save ? GetSaveFileNameW(&dialog) : GetOpenFileNameW(&dialog)) == FALSE) {
        return {};
    }
    return std::filesystem::path{path.data()};
}

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

/** Capture this application's owned visible window only, never the desktop or another process. */
void screenshot(HWND window, const std::filesystem::path& destination) {
    RECT bounds{};
    GetWindowRect(window, &bounds);
    const auto width = bounds.right - bounds.left;
    const auto height = bounds.bottom - bounds.top;
    const auto screen = GetDC(window);
    const auto memory = CreateCompatibleDC(screen);
    const auto bitmap = CreateCompatibleBitmap(screen, width, height);
    const auto previous = SelectObject(memory, bitmap);
    const auto painted = PrintWindow(window, memory, 2U);
    SelectObject(memory, previous);
    DeleteDC(memory);
    ReleaseDC(window, screen);
    if (painted == FALSE) {
        DeleteObject(bitmap);
        throw std::runtime_error("Owned window screenshot was unavailable.");
    }
    Gdiplus::GdiplusStartupInput startup;
    ULONG_PTR token{};
    require(Gdiplus::GdiplusStartup(&token, &startup, nullptr) == Gdiplus::Ok,
            "Screenshot encoder unavailable.");
    bool saved{};
    {
        // The PNG encoder has a stable system CLSID; it does not load an application plugin.
        const CLSID encoder{0x557cf406, 0x1a04, 0x11d3, {0x9a, 0x73, 0, 0, 0xf8, 0x1e, 0xf3, 0x2e}};
        Gdiplus::Bitmap image{bitmap, nullptr};
        saved = image.Save(destination.c_str(), &encoder, nullptr) == Gdiplus::Ok;
    }
    Gdiplus::GdiplusShutdown(token);
    DeleteObject(bitmap);
    require(saved, "Screenshot could not be saved.");
}

/** Main-window state. Session state, disk profile and visible source buffer remain separate. */
class Application {
  public:
    Application(std::filesystem::path profile, std::filesystem::path acceptance, int phase)
        : profile_(std::move(profile)), acceptance_(std::move(acceptance)), phase_(phase),
          owner_(CreateFileW((profile_ / L"owner.lock").c_str(), GENERIC_READ | GENERIC_WRITE, 0,
                             nullptr, OPEN_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OPEN_REPARSE_POINT, nullptr)) {
        require(owner_.get() != INVALID_HANDLE_VALUE,
                "This profile is already open or unavailable.");
        BY_HANDLE_FILE_INFORMATION info{};
        require(GetFileInformationByHandle(owner_.get(), &info) != FALSE &&
                    info.nNumberOfLinks == 1U &&
                    (info.dwFileAttributes &
                     (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) == 0U,
                "Unsafe profile lock.");
        const auto defaults = read_text(executable_directory() / L"assets" / L"arena.scene");
        session_ = std::make_unique<EditorSession>(defaults);
        const auto saved = profile_ / L"project.scene";
        if (std::filesystem::exists(saved)) {
            try {
                session_->apply_scene(read_text(saved));
            } catch (const std::exception&) {
                load_error_ =
                    true; // Preserve corrupt input; save requires an explicit new decision.
            }
        }
    }

    int run(HINSTANCE instance, int show) {
        WNDCLASSW main_class{};
        main_class.hInstance = instance;
        main_class.lpfnWndProc = procedure;
        main_class.lpszClassName = L"Forge2DEditorWindow";
        main_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        main_class.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
        require(RegisterClassW(&main_class) != 0U, "Cannot register editor window.");
        WNDCLASSW canvas_class = main_class;
        canvas_class.lpfnWndProc = canvas_procedure;
        canvas_class.lpszClassName = L"Forge2DEditorCanvas";
        canvas_class.hbrBackground = nullptr;
        require(RegisterClassW(&canvas_class) != 0U, "Cannot register canvas.");
        window_ = CreateWindowExW(0, main_class.lpszClassName, L"Forge2D Editor 1.1.0 - Jason Stys",
                                  WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT,
                                  CW_USEDEFAULT, 1240, 850, nullptr, nullptr, instance, this);
        require(window_ != nullptr, "Cannot create editor window.");
        ShowWindow(window_, show);
        UpdateWindow(window_);
        if (!acceptance_.empty()) {
            SetTimer(window_, acceptance_timer, 200U, nullptr);
        }
        MSG message{};
        while (GetMessageW(&message, nullptr, 0, 0) > 0) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        return exit_code_;
    }

  private:
    static LRESULT CALLBACK procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
        auto* app = reinterpret_cast<Application*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            const auto* create = reinterpret_cast<CREATESTRUCTW*>(lparam);
            app = static_cast<Application*>(create->lpCreateParams);
            app->window_ = window;
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        }
        if (app != nullptr) {
            try {
                return app->message(message, wparam, lparam);
            } catch (const std::exception& error) {
                app->session_->pause();
                app->notice_ = wide(error.what());
                if (!app->acceptance_.empty()) {
                    app->exit_code_ = 2;
                    atomic_text(app->acceptance_ / L"failure.txt", error.what());
                    DestroyWindow(window);
                } else {
                    MessageBoxW(window, app->notice_.c_str(), L"Forge2D - input not applied",
                                MB_OK | MB_ICONWARNING);
                    app->update_status();
                }
                return 0;
            }
        }
        return DefWindowProcW(window, message, wparam, lparam);
    }

    static LRESULT CALLBACK canvas_procedure(HWND window, UINT message, WPARAM wparam,
                                             LPARAM lparam) {
        auto* app = reinterpret_cast<Application*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            const auto* create = reinterpret_cast<CREATESTRUCTW*>(lparam);
            app = static_cast<Application*>(create->lpCreateParams);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        }
        if (app == nullptr) {
            return DefWindowProcW(window, message, wparam, lparam);
        }
        if (message == WM_LBUTTONDOWN) {
            SetFocus(window);
            return 0;
        }
        if (message == WM_KILLFOCUS) {
            app->keys_.fill(false);
        }
        if ((message == WM_KEYDOWN || message == WM_KEYUP) && wparam < app->keys_.size()) {
            app->keys_[wparam] = message == WM_KEYDOWN;
            return 0;
        }
        if (message == WM_PAINT) {
            try {
                app->paint_canvas(window);
            } catch (...) {
                app->session_->pause();
            }
            return 0;
        }
        return DefWindowProcW(window, message, wparam, lparam);
    }

    LRESULT message(UINT kind, WPARAM wparam, LPARAM lparam) {
        switch (kind) {
        case WM_CREATE:
            create_controls();
            return 0;
        case WM_SIZE:
            layout();
            return 0;
        case WM_GETMINMAXINFO: {
            auto* size = reinterpret_cast<MINMAXINFO*>(lparam);
            size->ptMinTrackSize = POINT{900, 650};
            return 0;
        }
        case WM_COMMAND:
            if (LOWORD(wparam) == text_id && HIWORD(wparam) == EN_CHANGE && !setting_text_) {
                unapplied_ = true;
                update_status();
            } else if (HIWORD(wparam) == 0U) {
                command(LOWORD(wparam));
            }
            return 0;
        case WM_TIMER:
            if (wparam == acceptance_timer) {
                KillTimer(window_, acceptance_timer);
                acceptance();
            } else if (wparam == frame_timer) {
                const auto now = std::chrono::steady_clock::now();
                const auto ticks = clock_.consume(now - last_frame_);
                last_frame_ = now;
                const auto previous_tick = session_->world().tick();
                for (std::size_t tick = 0; tick < ticks; ++tick) {
                    session_->advance(input());
                }
                if (session_->world().tick() != previous_tick) {
                    update_status();
                }
            }
            return 0;
        case WM_CLOSE:
            if (acceptance_.empty() && (unapplied_ || unsaved_) &&
                MessageBoxW(window_, L"Unapplied or unsaved edits will be discarded. Close?",
                            L"Forge2D - preserve your work",
                            MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING) != IDYES) {
                return 0;
            }
            DestroyWindow(window_);
            return 0;
        case WM_DESTROY:
            KillTimer(window_, frame_timer);
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(window_, kind, wparam, lparam);
        }
    }

    HWND child(const wchar_t* type, const wchar_t* text, DWORD style, int id) {
        const auto control = CreateWindowExW(
            type == std::wstring_view{L"EDIT"} ? WS_EX_CLIENTEDGE : 0, type, text,
            WS_CHILD | WS_VISIBLE | style, 0, 0, 1, 1, window_,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), GetModuleHandleW(nullptr), nullptr);
        require(control != nullptr, "Cannot create editor control.");
        SendMessageW(control, WM_SETFONT,
                     reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
        return control;
    }

    void create_controls() {
        const auto menu = CreateMenu();
        const auto file = CreatePopupMenu();
        AppendMenuW(file, MF_STRING, open_id, L"Open scene...");
        AppendMenuW(file, MF_STRING, save_id, L"Save applied project to profile");
        AppendMenuW(file, MF_STRING, replay_id, L"Import replay...");
        AppendMenuW(file, MF_STRING, export_id, L"Export evidence bundle...");
        AppendMenuW(file, MF_STRING, quit_id, L"Exit");
        AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(file), L"File");
        const auto examples = CreatePopupMenu();
        AppendMenuW(examples, MF_STRING, arena_id, L"Prepared arena");
        AppendMenuW(examples, MF_STRING, layers_id, L"Collision layers");
        AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(examples), L"Prepared data");
        AppendMenuW(menu, MF_STRING, help_id, L"Help / About");
        SetMenu(window_, menu);
        const std::array<std::pair<int, const wchar_t*>, 6> buttons{
            {{apply_id, L"Apply scene"},
             {play_id, L"Play / Pause"},
             {step_id, L"Step"},
             {reset_id, L"Reset"},
             {save_id, L"Save project"},
             {export_id, L"Export evidence"}}};
        for (const auto& [id, label] : buttons) {
            buttons_.push_back(child(L"BUTTON", label, WS_TABSTOP | BS_PUSHBUTTON, id));
        }
        heading_ =
            child(L"STATIC", L"Applied world | click here, then WASD / arrows / Space", 0, 0);
        source_label_ =
            child(L"STATIC", L"Scene source | edit, then Apply (all fields are editable)", 0, 0);
        source_ = child(L"EDIT", L"",
                        ES_MULTILINE | ES_WANTRETURN | ES_AUTOVSCROLL | ES_AUTOHSCROLL |
                            WS_VSCROLL | WS_HSCROLL | WS_TABSTOP,
                        text_id);
        SendMessageW(source_, EM_SETLIMITTEXT, EditorSession::maximum_input_bytes, 0);
        SendMessageW(source_, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(ANSI_FIXED_FONT)),
                     TRUE);
        canvas_ = CreateWindowExW(WS_EX_CLIENTEDGE, L"Forge2DEditorCanvas", L"Actual engine canvas",
                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 1, 1, window_, nullptr,
                                  GetModuleHandleW(nullptr), this);
        require(canvas_ != nullptr, "Cannot create engine canvas.");
        instructions_ =
            child(L"STATIC",
                  L"Save keeps the applied authored scene; replay records actual tick inputs.\r\n"
                  L"Collision masks detect overlaps, not physical response. File > Export evidence "
                  L"includes the matching scene.\r\n"
                  L"Local OS profile, no network. Created by Jason Stys | github.com/JasonStys | "
                  L"Help / About explains fields.",
                  0, 0);
        status_ = child(L"STATIC", L"", SS_LEFT, 0);
        set_source(session_->scene_text());
        if (load_error_) {
            notice_ = L"Saved project invalid: preserved on disk; working from prepared scene. "
                      L"Export recovery before overwriting.";
        } else if (!std::filesystem::exists(profile_ / L"project.scene")) {
            notice_ = L"First use: choose prepared data, edit and Apply, click canvas, Play. Save "
                      L"project persists it.";
        }
        last_frame_ = std::chrono::steady_clock::now();
        SetTimer(window_, frame_timer, 16U, nullptr);
        layout();
        update_status();
    }

    void layout() {
        RECT bounds{};
        GetClientRect(window_, &bounds);
        const auto width = std::max(880L, bounds.right);
        const auto height = std::max(580L, bounds.bottom);
        const auto split = width * 54 / 100;
        for (std::size_t index = 0; index < buttons_.size(); ++index) {
            MoveWindow(buttons_[index], 12 + static_cast<int>(index) * 137, 10, 129, 32, TRUE);
        }
        MoveWindow(heading_, 12, 53, split - 24, 24, TRUE);
        MoveWindow(source_label_, split + 8, 53, width - split - 20, 24, TRUE);
        MoveWindow(canvas_, 12, 81, split - 24, height - 216, TRUE);
        MoveWindow(source_, split + 8, 81, width - split - 20, height - 216, TRUE);
        MoveWindow(status_, 12, height - 125, width - 24, 53, TRUE);
        MoveWindow(instructions_, 12, height - 66, width - 24, 62, TRUE);
    }

    void set_source(std::string_view text) {
        // A native multiline EDIT requires CRLF; scene serialization deliberately remains portable
        // LF.
        std::string displayed;
        displayed.reserve(text.size() +
                          static_cast<std::size_t>(std::count(text.begin(), text.end(), '\n')));
        for (const auto character : text) {
            if (character == '\r') {
                continue;
            }
            if (character == '\n') {
                displayed += '\r';
            }
            displayed += character;
        }
        setting_text_ = true;
        require(SetWindowTextW(source_, wide(displayed).c_str()) != FALSE,
                "Cannot display scene source.");
        setting_text_ = false;
        unapplied_ = false;
    }

    [[nodiscard]] InputFrame input() const {
        const auto horizontal = static_cast<int>(keys_[VK_RIGHT] || keys_['D']) -
                                static_cast<int>(keys_[VK_LEFT] || keys_['A']);
        const auto vertical = static_cast<int>(keys_[VK_DOWN] || keys_['S']) -
                              static_cast<int>(keys_[VK_UP] || keys_['W']);
        return {static_cast<std::int8_t>(horizontal), static_cast<std::int8_t>(vertical),
                keys_[VK_SPACE]};
    }

    void update_status() {
        const auto& world = session_->world();
        std::wostringstream text;
        text << (session_->playing() ? L"PLAYING" : L"PAUSED") << L" | tick " << world.tick()
             << L" | entities " << world.entity_count() << L" | overlap pairs "
             << world.collision_pairs().size() << L" | recorded "
             << session_->recording().frames.size() << L"/36000"
             << (session_->reviewing_replay() ? L" | imported replay" : L" | live input")
             << (unapplied_ ? L" | UNAPPLIED EDITS" : L"") << (unsaved_ ? L" | UNSAVED" : L"")
             << L" | dropped wall ticks " << clock_.dropped_ticks() << L"\r\n"
             << notice_;
        SetWindowTextW(status_, text.str().c_str());
        InvalidateRect(canvas_, nullptr, FALSE);
    }

    void command(int id) {
        notice_.clear();
        if (id == apply_id) {
            session_->apply_scene(editor_text(source_));
            unapplied_ = false;
            unsaved_ = true;
        } else if (id == play_id) {
            if (session_->playing()) {
                session_->pause();
            } else {
                session_->play();
                SetFocus(canvas_);
            }
        } else if (id == step_id) {
            session_->pause();
            session_->step(input());
        } else if (id == reset_id) {
            session_->reset();
        } else if (id == save_id) {
            require(!unapplied_, "Apply or discard the pending source edits before saving.");
            if (load_error_ && acceptance_.empty() &&
                MessageBoxW(window_,
                            L"The saved project was invalid and has been preserved. Replace it "
                            L"with the applied scene?",
                            L"Confirm project replacement",
                            MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING) != IDYES) {
                return;
            }
            atomic_text(profile_ / L"project.scene", session_->scene_text());
            unsaved_ = false;
            load_error_ = false;
            notice_ = L"Applied project saved in your OS-local profile. Running state is not "
                      L"auto-resumed.";
        } else if (id == open_id || id == arena_id || id == layers_id) {
            if ((unapplied_ || unsaved_) && acceptance_.empty() &&
                MessageBoxW(
                    window_,
                    L"Replace the editor buffer and applied scene, discarding unsaved edits?",
                    L"Open scene", MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING) != IDYES) {
                return;
            }
            auto path =
                id == open_id
                    ? choose_file(window_, false, L"Forge2D scene\0*.scene\0All files\0*.*\0")
                    : executable_directory() / L"assets" /
                          (id == arena_id ? L"arena.scene" : L"collision-layers.scene");
            if (!path.empty()) {
                session_->apply_scene(read_text(path));
                set_source(session_->scene_text());
                unsaved_ = true;
            }
        } else if (id == replay_id) {
            const auto path =
                choose_file(window_, false, L"Forge2D replay\0*.replay\0All files\0*.*\0");
            if (!path.empty()) {
                session_->load_replay(read_text(path));
                notice_ = L"Replay loaded against the applied scene. Play or Step consumes its "
                          L"recorded inputs.";
            }
        } else if (id == export_id) {
            const auto path = choose_file(window_, true, L"Evidence bundle base\0*.scene\0");
            if (!path.empty()) {
                const auto directory = path.parent_path() / (path.stem().wstring() + L".evidence");
                export_bundle(directory);
                notice_ = L"Evidence folder created; existing folders are never overwritten.";
            }
        } else if (id == help_id) {
            MessageBoxW(window_, help_text, L"Forge2D - how this editor works",
                        MB_OK | MB_ICONINFORMATION);
        } else if (id == quit_id) {
            SendMessageW(window_, WM_CLOSE, 0, 0);
            return;
        }
        update_status();
    }

    void export_bundle(const std::filesystem::path& path) const {
        require(std::filesystem::create_directory(path),
                "Choose a new evidence folder; existing exports are preserved.");
        atomic_text(path / L"scene.scene", session_->scene_text());
        atomic_text(path / L"inputs.replay", session_->replay_text());
        const auto image = session_->image(640U, 640U);
        image.write_ppm(path / L"frame.ppm");
        std::ostringstream csv;
        csv << "entity_id,player,x,y,vx,vy,category,mask\n";
        for (const auto& entity : session_->world().snapshot()) {
            csv << entity.entity.packed() << ',' << static_cast<int>(entity.player) << ','
                << entity.transform.position.x << ',' << entity.transform.position.y << ','
                << entity.body.velocity.x << ',' << entity.body.velocity.y << ','
                << entity.body.filter.category << ',' << entity.body.filter.mask << '\n';
        }
        atomic_text(path / L"snapshot.csv", csv.str());
        std::ostringstream report;
        report << "{\"schema\":1,\"version\":\"" << FORGE2D_VERSION
               << "\",\"tick\":" << session_->world().tick() << ",\"stateHash\":\""
               << session_->world().state_hash() << "\",\"pixelHash\":\"" << image.pixel_hash()
               << "\",\"overlapPairs\":" << session_->world().collision_pairs().size()
               << ",\"recordedFrames\":" << session_->recording().frames.size()
               << ",\"author\":\"Jason Stys\"}\n";
        atomic_text(path / L"summary.json", report.str());
    }

    void paint_canvas(HWND canvas) {
        PAINTSTRUCT paint{};
        const auto dc = BeginPaint(canvas, &paint);
        RECT bounds{};
        GetClientRect(canvas, &bounds);
        if (bounds.right > 0 && bounds.bottom > 0) {
            const auto image = session_->image(640U, 640U);
            std::vector<std::uint32_t> pixels;
            pixels.reserve(image.pixels().size());
            for (const auto rgba : image.pixels()) {
                pixels.push_back(rgba >> 8U);
            }
            BITMAPINFO info{};
            info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            info.bmiHeader.biWidth = 640;
            info.bmiHeader.biHeight = -640;
            info.bmiHeader.biPlanes = 1;
            info.bmiHeader.biBitCount = 32;
            info.bmiHeader.biCompression = BI_RGB;
            StretchDIBits(dc, 0, 0, bounds.right, bounds.bottom, 0, 0, 640, 640, pixels.data(),
                          &info, DIB_RGB_COLORS, SRCCOPY);
        }
        EndPaint(canvas, &paint);
    }

    /** Event-dispatched synthetic acceptance exercises this same visible executable and handlers.
     */
    void acceptance() {
        require(IsWindowVisible(window_) != FALSE && IsWindowVisible(canvas_) != FALSE,
                "Native editor window is not visible.");
        if (phase_ == 1) {
            if (acceptance_play_pending_) {
                if (session_->playing() && GetTickCount64() - acceptance_play_started_ < 2'000U) {
                    SetTimer(window_, acceptance_timer, 16U, nullptr);
                    return;
                }
                require(!session_->playing() && session_->world().tick() == 3U &&
                            session_->world().state_hash() == acceptance_replay_hash_,
                        "Native Play timer did not complete the three-frame imported replay.");
                finish_acceptance();
                return;
            }
            command(arena_id);
            require(SendMessageW(source_, EM_GETLINECOUNT, 0, 0) >= 10,
                    "Native source control did not display the actual multiline scene.");
            const Handle duplicate{CreateFileW((profile_ / L"owner.lock").c_str(), GENERIC_READ, 0,
                                               nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                               nullptr)};
            require(duplicate.get() == INVALID_HANDLE_VALUE,
                    "Profile owner lock was not exclusive.");
            auto authored = editor_text(source_);
            const auto location = authored.find("750 750");
            require(location != std::string::npos, "Prepared scene coordinate missing.");
            authored.replace(location, 7, "1100 900");
            SetWindowTextW(source_, wide(authored).c_str());
            SendMessageW(window_, WM_COMMAND, apply_id, 0);
            SendMessageW(window_, WM_COMMAND, save_id, 0);
            require(!unapplied_ && !unsaved_,
                    "Applied scene not saved through the actual handler.");
            const auto saved = read_text(profile_ / L"project.scene");
            const auto before = session_->world().state_hash();
            SetWindowTextW(source_, L"FORGE2D_SCENE invalid");
            bool refused{};
            try {
                command(apply_id);
            } catch (const std::exception&) {
                refused = true;
            }
            require(refused && session_->world().state_hash() == before &&
                        read_text(profile_ / L"project.scene") == saved,
                    "Invalid scene changed applied world or saved file.");
            set_source(saved);
            SetFocus(canvas_);
            SendMessageW(canvas_, WM_KEYDOWN, VK_RIGHT, 0);
            for (int tick = 0; tick < 3; ++tick) {
                SendMessageW(window_, WM_COMMAND, step_id, 0);
            }
            SendMessageW(canvas_, WM_KEYUP, VK_RIGHT, 0);
            const auto snapshot = session_->world().snapshot();
            const auto player = std::find_if(snapshot.begin(), snapshot.end(),
                                             [](const auto& entity) { return entity.player; });
            require(player != snapshot.end() && player->transform.position == Vec2i{1520, 900},
                    "Actual engine did not execute the three captured right inputs.");
            const auto hash = session_->world().state_hash();
            const auto replay = session_->replay_text();
            export_bundle(acceptance_ / L"evidence");
            const auto initial = World::from_scene(
                parse_scene(read_text(acceptance_ / L"evidence" / L"scene.scene")));
            require(run_replay(initial, parse_replay(read_text(acceptance_ / L"evidence" /
                                                               L"inputs.replay"))) == hash,
                    "Exported scene/replay does not independently reproduce the state.");
            session_->load_replay(replay);
            acceptance_replay_hash_ = hash;
            acceptance_play_pending_ = true;
            acceptance_play_started_ = GetTickCount64();
            SendMessageW(window_, WM_COMMAND, play_id, 0);
            SetTimer(window_, acceptance_timer, 16U, nullptr);
            return;
        } else if (phase_ == 2) {
            require(!load_error_, "Saved project did not restore.");
            const auto snapshot = session_->world().snapshot();
            const auto player = std::find_if(snapshot.begin(), snapshot.end(),
                                             [](const auto& entity) { return entity.player; });
            require(player != snapshot.end() && player->transform.position == Vec2i{1100, 900} &&
                        session_->world().tick() == 0U && !session_->playing(),
                    "Restart did not restore authored coordinates paused at tick zero.");
            atomic_text(acceptance_ / L"phase-2.json",
                        "{\"schema\":1,\"version\":\"1.1.0\",\"restoredAuthoredScene\":true,"
                        "\"tick\":0,\"playing\":false}\n");
        } else {
            require(load_error_ && !session_->playing() &&
                        read_text(profile_ / L"project.scene") == "invalid saved scene\n" &&
                        notice_.find(L"preserved on disk") != std::wstring::npos,
                    "Corrupt saved project was reset or not visibly identified.");
            atomic_text(acceptance_ / L"phase-3.json",
                        "{\"schema\":1,\"version\":\"1.1.0\",\"corruptProjectPreserved\":true,"
                        "\"visibleRecoveryNotice\":true,\"preparedWorkingCopy\":true}\n");
        }
        unapplied_ = false;
        unsaved_ = false;
        SendMessageW(window_, WM_COMMAND, quit_id, 0);
    }

    void finish_acceptance() {
        notice_ = L"Acceptance: applied custom coordinates, refused invalid input, saved, played "
                  L"three inputs and verified exported replay.";
        update_status();
        UpdateWindow(canvas_);
        UpdateWindow(window_);
        screenshot(window_, acceptance_ / L"editor-play.png");
        command(layers_id);
        notice_ = L"Prepared collision-layer scene: category/mask fields are editable in the same "
                  L"application.";
        update_status();
        UpdateWindow(canvas_);
        screenshot(window_, acceptance_ / L"editor-layers.png");
        bool overwrite_refused{};
        try {
            export_bundle(acceptance_ / L"evidence");
        } catch (const std::exception&) {
            overwrite_refused = true;
        }
        require(overwrite_refused && read_text(acceptance_ / L"evidence" / L"scene.scene") ==
                                         read_text(profile_ / L"project.scene"),
                "Existing export was overwritten.");
        set_source("FORGE2D_SCENE 2\nbounds 0 0 1000 1000\nentities 0\n");
        command(apply_id);
        command(step_id);
        require(session_->world().entity_count() == 0U && session_->world().tick() == 1U,
                "Empty scene could not be applied and stepped through the native controls.");
        std::ostringstream receipt;
        receipt
            << "{\"schema\":1,\"version\":\"1.1.0\",\"visibleWindow\":true,\"customScene\":true,"
               "\"invalidPreserved\":true,\"realInputs\":3,\"player\":[1520,900],"
               "\"exportReplayVerified\":true,\"nativeHandlers\":true,\"nativePlayTimer\":true,"
               "\"profileLockExclusive\":true,\"multilineSource\":true,\"exportOverwriteRefused\":"
               "true,\"emptyScene\":true,\"source\":\""
            << FORGE2D_NATIVE_SOURCE
            << "\",\"sourceDirty\":" << (FORGE2D_NATIVE_DIRTY ? "true" : "false") << "}\n";
        atomic_text(acceptance_ / L"phase-1.json", receipt.str());
        unapplied_ = false;
        unsaved_ = false;
        SendMessageW(window_, WM_COMMAND, quit_id, 0);
    }

    std::filesystem::path profile_;
    std::filesystem::path acceptance_;
    int phase_{};
    Handle owner_;
    std::unique_ptr<EditorSession> session_;
    HWND window_{};
    HWND canvas_{};
    HWND source_{};
    HWND heading_{};
    HWND source_label_{};
    HWND instructions_{};
    HWND status_{};
    std::vector<HWND> buttons_;
    std::array<bool, 256> keys_{};
    bool setting_text_{};
    bool unapplied_{};
    bool unsaved_{};
    bool load_error_{};
    int exit_code_{};
    bool acceptance_play_pending_{};
    ULONGLONG acceptance_play_started_{};
    std::uint64_t acceptance_replay_hash_{};
    std::wstring notice_;
    FixedStepClock clock_{std::chrono::nanoseconds{1'000'000'000 / 60}};
    std::chrono::steady_clock::time_point last_frame_{};
};
} // namespace
} // namespace forge2d::desktop

/** No user-selected arguments are executed. Acceptance roots require an explicit private marker. */
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    using namespace forge2d::desktop;
    std::filesystem::path acceptance;
    try {
        int count{};
        auto** arguments = CommandLineToArgvW(GetCommandLineW(), &count);
        require(arguments != nullptr, "Cannot read application arguments.");
        std::filesystem::path profile;
        int phase{};
        if (count == 1) {
            profile = default_profile();
        } else if (count == 5 && std::wstring_view{arguments[1]} == L"--acceptance-root" &&
                   std::wstring_view{arguments[3]} == L"--acceptance-phase") {
            std::array<wchar_t, 8> enabled{};
            require(GetEnvironmentVariableW(L"FORGE2D_ACCEPTANCE", enabled.data(),
                                            static_cast<DWORD>(enabled.size())) == 1U &&
                        enabled[0] == L'1',
                    "Acceptance mode is not enabled.");
            acceptance = std::filesystem::absolute(arguments[2]);
            require(read_text(acceptance / L"acceptance.marker") == "forge2d-owned-native-test\n",
                    "Acceptance root marker is missing.");
            const std::wstring_view choice{arguments[4]};
            require(choice == L"1" || choice == L"2" || choice == L"3",
                    "Invalid acceptance phase.");
            phase = choice == L"1" ? 1 : (choice == L"2" ? 2 : 3);
            profile = acceptance / L"profile";
        } else {
            LocalFree(arguments);
            throw std::invalid_argument(
                "This application does not accept arbitrary command-line arguments.");
        }
        LocalFree(arguments);
        ensure_profile(profile);
        Application app{profile, acceptance, phase};
        return app.run(instance, show);
    } catch (const std::exception& error) {
        if (acceptance.empty()) {
            MessageBoxW(nullptr, wide(error.what()).c_str(), L"Forge2D Editor could not start",
                        MB_OK | MB_ICONERROR);
        } else {
            try {
                atomic_text(acceptance / L"startup-failure.txt", error.what());
            } catch (...) {
                // A failure cannot justify writing elsewhere or displaying a modal test dialog.
            }
        }
        return 2;
    }
}
