# CD-i Menu Recomp

Standalone player for the Philips CD-i system menu. This repo owns the launcher, native HLE build profile and release archive. The pinned [cdirecomp](https://github.com/mstan/cdirecomp) submodule provides the CD-RTOS recompiler and player runtime.

## Windows preview

Extract the release ZIP and open **CdiMenu.exe**. In recomp-ui, select your original CD-i player ROM matching the hash listed in `build.json` and select **Play**. The launcher starts the original CD-RTOS shell without a disc. A controller, mouse or the arrow keys can move the cursor. Z, Enter or Space selects an item.

The package needs the user-supplied player ROM. It contains the native runtime, launcher, runtime libraries and notices. The settings and original NVRAM are kept by the engine under its per-user CD-i player directory.

## Build from an original player ROM

Use Python 3.11 or later and MinGW-w64 with CMake, Ninja and SDL2. Initialize the engine and compiler pins recursively. The build driver reads the original ROM and generates native code in the private work directory.

```powershell
git submodule update --init --recursive
python -B tools/build.py --firmware D:/Private/cdi490a.rom --work-root G:/Temp/CDiMenuBuild
python -B tools/package.py --firmware D:/Private/cdi490a.rom --work-root G:/Temp/CDiMenuBuild --out G:/Temp/CDiMenuRecomp-windows-x64.zip
```

Launcher code is MIT licensed. The engine components retain the [cdirecomp license](cdirecomp/LICENSE) and [third-party notices](cdirecomp/THIRD-PARTY-NOTICES.md).
