#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/../.." && pwd)"
: "${SDL_INCLUDE:?Set SDL_INCLUDE to the directory containing SDL3 headers}"
work="$(mktemp -d)"
trap 'rm -rf -- "$work"' EXIT
common=(-O2 -Wall -Wextra -ffunction-sections -fdata-sections -I"$root/src" -I"$root/aeron/include" -I"$SDL_INCLUDE")
cc "${common[@]}" "$root/tests/rgsp/test-scene.c" "$root/src/xw_remaster/cockpit_scene_lighting.c" "$root/src/xw_remaster/render_math.c" -Wl,--gc-sections -lm -o "$work/scene"
cc "${common[@]}" "$root/tests/rgsp/test-scene-integration.c" "$root/src/xw_remaster/cockpit_lighting.c" "$root/src/xw_remaster/cockpit_scene_lighting.c" "$root/src/xw_remaster/render_math.c" -Wl,--gc-sections -lm -o "$work/integration"
cd "$work"
./scene
./integration
./integration reload
./integration failure
