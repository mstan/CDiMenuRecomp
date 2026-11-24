# SPDX-License-Identifier: MIT
"""Package the Windows CD-i Menu preview and audit its dependencies."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import zipfile

from build import ENGINE, ROOT

sys.path.insert(0, str(ENGINE / "tools"))
from build_provenance import verify
from package_runtime_release import audit_build_graph, audit_runtime, runtime_dependencies


def digest(path):
    with path.open("rb") as file:
        return hashlib.file_digest(file, "sha256").hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--firmware", required=True, type=Path)
    parser.add_argument("--work-root", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    parser.add_argument("--toolchain-bin", type=Path, default=Path("C:/msys64/mingw64/bin"))
    args = parser.parse_args()
    os.environ["PATH"] = str(args.toolchain_bin) + os.pathsep + os.environ["PATH"]
    menu = args.work_root.resolve() / "menu"
    runtime = menu / "runtime/CdiMenuRuntime.exe"
    launcher = menu / "CdiMenu.exe"
    expected_rom = digest(args.firmware)
    identity = args.work_root / "firmware/bios_identity.h"
    compiled_sha = "".join(re.findall(r"0x([a-f0-9]{2})", identity.read_text().lower()))
    if expected_rom != compiled_sha:
        raise ValueError("The selected ROM differs from the compiled BIOS identity")
    audit_build_graph(menu / runtime.name, "CdiMenuRuntime")
    libraries = runtime_dependencies(runtime, args.toolchain_bin / "SDL2.dll", [args.toolchain_bin])
    audit_runtime(runtime, {name.lower() for name in libraries})
    verify(runtime)
    files = {"CdiMenu.exe": launcher, "runtime/CdiMenuRuntime.exe": runtime,
             "README.md": ROOT / "README.md", "LICENSE": ROOT / "LICENSE",
             "licenses/cdirecomp-LICENSE": ENGINE / "LICENSE",
             "licenses/cdirecomp-PERMISSIONS.md": ENGINE / "PERMISSIONS.md",
             "licenses/THIRD-PARTY-NOTICES.md": ENGINE / "THIRD-PARTY-NOTICES.md"}
    files.update({f"runtime/{name}": path for name, path in libraries.items()})
    metadata = {
        "version": "0.1.0", "firmware_sha256": expected_rom,
        "source": subprocess.check_output(["git", "-C", ROOT, "rev-parse", "HEAD"], text=True).strip(),
        "cdirecomp": subprocess.check_output(["git", "-C", ENGINE, "rev-parse", "HEAD"], text=True).strip(),
        "files": {name: digest(path) for name, path in files.items()},
    }
    args.out.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(args.out, "x", zipfile.ZIP_DEFLATED) as archive:
        for name, path in files.items():
            archive.write(path, name)
        archive.writestr("build.json", json.dumps(metadata, indent=2) + "\n")
    with zipfile.ZipFile(args.out) as archive:
        if archive.testzip() is not None:
            raise ValueError("Package integrity check failed")
    print(f"{digest(args.out)}  {args.out}")


if __name__ == "__main__":
    main()
