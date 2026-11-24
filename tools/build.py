# SPDX-License-Identifier: MIT
"""Generate the user-owned CD-i player ROM and build the HLE menu."""

import argparse
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / "cdirecomp"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--firmware", required=True, type=Path)
    parser.add_argument("--work-root", required=True, type=Path)
    parser.add_argument("--toolchain-bin", type=Path, default=Path("C:/msys64/mingw64/bin"))
    parser.add_argument("--jobs", type=int, default=2)
    args = parser.parse_args()
    work, firmware = args.work_root.resolve(), args.firmware.resolve()
    if work.is_relative_to(ROOT) or not firmware.is_file() or args.jobs < 1:
        parser.error("Use an existing player ROM, an out-of-tree work directory and positive jobs")
    env = dict(os.environ)
    cmake = "cmake"
    suffix = ""
    if os.name == "nt":
        env["PATH"] = str(args.toolchain_bin) + os.pathsep + env["PATH"]
        cmake = args.toolchain_bin / "cmake.exe"
        suffix = ".exe"

    def run(*command):
        subprocess.run(command, env=env, cwd=ROOT, check=True)

    emitter = work / "emitter"
    menu = work / "menu"
    generated = work / "firmware"
    generated.mkdir(parents=True, exist_ok=True)
    run(cmake, "-S", ENGINE / "recompiler", "-B", emitter, "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release")
    run(cmake, "--build", emitter, "--target", "CdiRecompBios", "--parallel", str(args.jobs))
    run(emitter / f"CdiRecompBios{suffix}", firmware, "--emit", "--out", generated,
        "--seeds", ENGINE / "bios/cdrtos_discovered.txt")
    configure = [cmake, "-S", ROOT, "-B", menu, "-G", "Ninja",
                 "-DCMAKE_BUILD_TYPE=Release", f"-DCDI_BIOS_GENERATED_DIR={generated}"]
    if os.name == "nt":
        configure.append(f"-DSDL2_DIR={args.toolchain_bin.parent / 'lib/cmake/SDL2'}")
    run(*configure)
    run(cmake, "--build", menu, "--parallel", str(args.jobs))


if __name__ == "__main__":
    main()
