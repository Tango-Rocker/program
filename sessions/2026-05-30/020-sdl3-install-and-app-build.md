# Session: SDL3 install and app build

## Goal
- Install SDL3 for the MSYS2 UCRT toolchain and verify the SDL app target builds.

## Files touched
- `src/ui/debug_overlay.c`
- `README.md`
- `sessions/2026-05-30/020-sdl3-install-and-app-build.md`

## Decisions
- Installed `mingw-w64-ucrt-x86_64-sdl3` with MSYS2 `pacman`, matching the existing `ucrt64` GCC/CMake toolchain.
- Updated the debug overlay to use `SDL_FRect`, matching SDL3's render API.
- Documented that PowerShell sessions need `C:\Users\Tango\tools\msys64\ucrt64\bin` on `PATH` before running `sim_app.exe`.

## Validation
- `cmake --build cmake-build-debug --target sim_app` succeeds when the UCRT bin path is present.
- `sim_app.exe` is produced at `cmake-build-debug/sim_app.exe`.

## Next tasks
- Add a richer SDL render pass for the default scene map and camera once visual debugging becomes the active task.
