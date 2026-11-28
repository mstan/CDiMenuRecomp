# SPDX-License-Identifier: MIT
"""Generate the user-owned CD-i player ROM and build the HLE menu."""

import argparse
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
ENGINE = ROOT / "cdirecomp"
sys.path.insert(0, str(ENGINE / "tools"))
from bios_generation import build_bios_emitter, generate_bios


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--firmware", required=True, type=Path)
    parser.add_argument("--work-root", required=True, type=Path)
    parser.add_argument("--toolchain-bin", type=Path, default=Path("C:/msys64/mingw64/bin"))
    parser.add_argument("--jobs", type=int, default=2)
    parser.add_argument("--tools-build-dir", type=Path, help="Explicit shared emitter build directory")
    args = parser.parse_args()
    work, firmware = args.work_root.resolve(), args.firmware.resolve()
    if work.is_relative_to(ROOT) or not firmware.is_file() or args.jobs < 1:
        parser.error("Use an existing player ROM, an out-of-tree work directory and positive jobs")
    env = dict(os.environ)
    cmake = "cmake"
    if os.name == "nt":
        env["PATH"] = str(args.toolchain_bin) + os.pathsep + env["PATH"]
        cmake = args.toolchain_bin / "cmake.exe"

    def run(*command):
        subprocess.run(command, env=env, cwd=ROOT, check=True)

    emitter = args.tools_build_dir or work / "emitter"
    menu = work / "menu"
    generated = work / "firmware"
    bios_emitter, toolchain = build_bios_emitter(ENGINE, emitter, cmake=cmake, jobs=args.jobs, env=env)
    generate_bios(ENGINE, firmware, bios_emitter, generated, toolchain=toolchain, env=env)
    configure = [cmake, "-S", ROOT, "-B", menu, "-G", "Ninja",
                 "-DCMAKE_BUILD_TYPE=Release", f"-DCDI_BIOS_GENERATED_DIR={generated.as_posix()}"]
    if os.name == "nt":
        configure.append(f"-DSDL2_DIR={(args.toolchain_bin.parent / 'lib/cmake/SDL2').as_posix()}")
    run(*configure)
    run(cmake, "--build", menu, "--parallel", str(args.jobs))


if __name__ == "__main__":
    main()
