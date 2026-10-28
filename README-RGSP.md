# OpenXW for Anbernic RGSP

This fork preserves the source used by our OpenXWing HD build for the Anbernic RGSP (H700 / Mali-G31).

## RGSP additions

- Controller command menu, editable bindings, D-pad steering assistance and optional aiming crosshair.
- Target browser with ship artwork, hull/system information and targeting controls.
- Energy, shield direction and firing controls; hold-to-look camera controls.
- HD model/texture loading, squadron paint masks and an HD/Original selection in View & Settings.
- Dynamic cockpit lighting with protected instruments, ship/asteroid sun occlusion, laser/ion and explosion illumination. Off, Subtle and Strong are available in View & Settings.
- Save slots and handheld frontend adjustments.

Cockpit lighting currently supports the original DOS94 X-Wing front cockpit. Blockers use original low-detail geometry; illumination is an approximation applied to the 2D artwork. At most eight blockers and four flash sources are evaluated. Side/rear views and unsupported cockpit artwork retain their original rendering.

## Dependencies and assets

Aeron and Landru are vendored here because the working RGSP build includes local changes to both. Their existing license files are retained. iMUSE remains a pinned submodule:

```sh
git clone --recurse-submodules https://github.com/VisionaiR3D/OpenXW_RGSP.git
```

Original commercial game data and the separately installed HD model/texture pack are not included. This repository contains the HD loading/rendering code, not those converted assets. Keep using the existing asset installation when updating the device executable.

See the upstream README for general build requirements. The verified handheld build uses an AArch64 Linux toolchain, SDL3, FFmpeg and SPIR-V shaders. Its environment-specific SDK/toolchain paths must be supplied locally; this source update does not provide a self-contained SDK or installer.

## Verification

The matching source was built for ARM64 and installed with SHA256 verification. Standalone scene and cockpit integration tests passed on the RGSP, including occlusion, projectile colors, source limits, pause/fade behavior and protected instruments. A four-source isolated calculation measured about 1.87 ms, excluding draw-list construction and GPU rendering; this is not a full-game frame benchmark.

Run the lighting tests on Linux with SDL3 headers available:

```sh
SDL_INCLUDE=/path/to/SDL3/include bash tests/rgsp/run-lighting-tests.sh
```

The latest blocker/projectile effects were checked in an offline render. Their in-flight appearance still needs manual verification. No game controls or restart are performed by these tests.
