"""File: smoke-native.py. Purpose: run actual play/restart/recovery with a marked synthetic profile.

Functions/variables and exact source lines are listed in docs/code-index.md. This harness uses
the editor's own event handlers, real engine, file IO and visible window, not external OS input.
It never reads the default user profile or installs shortcuts. Timeouts are failures, not retries.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile
import zipfile


def run(executable: Path, output: Path, metadata: dict | None = None) -> None:
    """Bind acceptance to one packaged EXE and fresh root; preserve all three actual launch results."""
    executable = executable.resolve(strict=True)
    output = output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    (output / "acceptance.marker").write_text("forge2d-owned-native-test\n", encoding="utf-8", newline="\n")
    environment = {key: os.environ[key] for key in ("SYSTEMROOT", "WINDIR", "TEMP", "TMP") if key in os.environ}
    environment.update(PATH=str(Path(os.environ.get("SYSTEMROOT", "C:/Windows")) / "System32"),
                       FORGE2D_ACCEPTANCE="1")
    stages = []
    with tempfile.TemporaryDirectory(prefix="forge2d-native-cwd-") as cwd:
        for phase in (1, 2, 3):
            if phase == 3:
                (output / "profile" / "project.scene").write_text("invalid saved scene\n", encoding="utf-8", newline="\n")
            result = subprocess.run(
                [str(executable), "--acceptance-root", str(output), "--acceptance-phase", str(phase)],
                cwd=cwd,
                env=environment,
                timeout=45,
                check=False,
            )
            if result.returncode != 0:
                raise RuntimeError(f"Native acceptance phase {phase} exited {result.returncode}.")
            stages.append(json.loads((output / f"phase-{phase}.json").read_text(encoding="utf-8")))
    if metadata is not None:
        if (metadata["source"] != stages[0]["source"] or metadata["dirty"] is not stages[0]["sourceDirty"]
                or metadata["version"] != stages[0]["version"]):
            raise RuntimeError("Packaged metadata does not match the actual compiled source/version.")
        if os.environ.get("CI") and metadata["dirty"] is not False:
            raise RuntimeError("Hosted native acceptance requires a clean source checkout.")
    summary = json.loads((output / "evidence" / "summary.json").read_text(encoding="utf-8"))
    if summary["recordedFrames"] != 3 or summary["tick"] != 3:
        raise RuntimeError("Export did not retain the actual three-step engine execution.")
    for name in ("editor-play.png", "editor-layers.png"):
        image = output / name
        if not image.read_bytes().startswith(b"\x89PNG\r\n\x1a\n") or image.stat().st_size < 10_000:
            raise RuntimeError("Native screenshot missing or implausibly empty.")
    receipt = {
        "schema": 1,
        "version": "1.1.0",
        "executableSha256": hashlib.sha256(executable.read_bytes()).hexdigest(),
        "phases": stages,
        "export": summary,
        "scope": "Actual visible native windows and own-handler input; no external mouse/shortcut or clean-machine proof.",
    }
    (output / "native-acceptance.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    if (output / "profile" / "project.scene").read_bytes() != b"invalid saved scene\n":
        raise RuntimeError("Corrupt-profile recovery overwrote the preserved synthetic input.")
    print(json.dumps({"conclusion": "success", "launches": 3, "recordedInputs": 3, "output": str(output)}))


def main() -> None:
    """Use only caller-named EXE/ZIP and a new output directory; archives are this build's outputs."""
    parser = argparse.ArgumentParser()
    parser.add_argument("executable", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    if args.executable.suffix.lower() == ".zip":
        with tempfile.TemporaryDirectory(prefix="forge2d-extracted-") as folder:
            with zipfile.ZipFile(args.executable) as package:
                names = set(package.namelist())
                allowed = {"Forge2DEditor.exe", "LICENSE", "RELEASE.json", "README.txt",
                           "assets/arena.scene", "assets/collision-layers.scene"}
                if names - {"assets/"} != allowed or sum(row.file_size for row in package.infolist()) > 30_000_000:
                    raise ValueError("Unexpected portable archive members or size.")
                package.extractall(folder)
            metadata = json.loads((Path(folder) / "RELEASE.json").read_text(encoding="utf-8-sig"))
            run(Path(folder) / "Forge2DEditor.exe", args.output, metadata)
    else:
        run(args.executable, args.output)


if __name__ == "__main__":
    main()
