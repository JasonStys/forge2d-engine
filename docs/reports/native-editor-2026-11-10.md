# Native editor validation — 2026-11-10

The local 1.1.0 candidate uses the original C++20/C17 engine, scene/replay parsers, collision filters
and software renderer. New `EditorSession` logic keeps the authored scene separate from the running
world and recorded inputs. Default renderer coordinates are unchanged; translated scene origins are
now supported explicitly and independently checked against the same relative pixel image.

Observed local MSVC Release validation: all **three CTest executables passed**. These comprise the
original ten core groups, four editor-model groups, and real Windows file-boundary cases. Tests
cover invalid/oversized input preserving prior world state; exact independently authored player
coordinates; zero-mask filtering; empty scenes; maximum entity/frame counts; imported replay rate,
completion and live-input override; translated rendering; actual atomic replacement; hardlink,
directory, invalid UTF-8 and oversized file refusal. Coverage percentages from an older report are
not reused for this new source; Linux coverage and full hosted gates remain pending.

The actual visible native EXE ran with a new marked synthetic profile. Acceptance edited
player coordinates through the real source control, Applied and Saved through the native handlers,
refused invalid input without changing disk/world, captured three right-input steps to **(1520,900)**,
and independently replayed the exported scene/input pair to the actual state hash. Native Play timer
events consumed that three-frame replay and stopped it. The source control's actual multiline row
count and exclusive profile lock were checked. Restart restored authored **(1100,900)** paused at
tick zero. A third launch preserved an intentionally malformed saved scene and displayed the real
recovery notice while offering a prepared working copy. Empty-scene Apply/Step and existing-export
refusal also passed through the real native workflow. All three processes exited normally. The
actual ZIP was separately extracted and passed the same three-launch journey with only Windows
system executables on PATH; its metadata matched the actual EXE's compiled source/dirty/version.
Only this application's owned window was captured, never the desktop.

The original 20,000-entity benchmark passed unchanged budgets: storage 1.4605 ms, collision 16.7956 ms
and simulation 11.4666 ms in this local run. These headless kernel measurements exclude native GUI,
file/export work and installation. The 360-tick original arena demo passed with eight entities;
state hash `7682307639504695130` and pixel hash `14365382764149795246` describe that specific input.

Screenshot inspection caught a genuine native EDIT presentation issue: portable LF text did not
display as separate Windows lines. Source display now converts to CRLF; retrieval converts back
before the strict parser. A subsequent actual multiline-control assertion caught the missing reverse
conversion; that was fixed before acceptance passed. The failed evidence remains private; no test
deadline, assertion or core oracle was relaxed. A packaging-shell hash cmdlet availability issue was
fixed with the standard .NET streaming SHA256 API, leaving package hashes independently verifiable.

The first PR integration at `03c0175` passed actual Windows extracted-package acceptance and the
Linux GCC/Clang, coverage, sanitizers/fuzz, clang-tidy, SDL and repository lanes. Two other Windows
jobs selected MinGW despite an old MSVC label; their Unicode `wWinMain` link required `-municode`.
The original Windows compiler lane is now explicitly GCC/MinGW and labelled accordingly; actual
MSVC remains exercised by native packaging. The container's explicit target list also omitted the
new editor-model test before running all CTests; that target is now built. These were real build/
workflow integration faults, corrected without disabling tests. Corrective hosted results remain
pending; Docker on the PC was not changed or used.

Boundaries: this is an unsigned Windows portable app, not an installer, signing proof, populated-user
migration or clean-machine certification. Actual own-window controls/timers were used; a human's
desktop-icon click, file-dialog selection, keyboard accessibility and every OS failure condition were
not automated. Only synthetic profiles were accessed. The app adds no network, hardware, scripting
or physical collision response. Recorded inputs require their matching authored scene. Local files
remain visible to the same OS user/administrator. Historical Word editions retain their original
source baselines until the separately reviewed updated guide is produced.
