# Advanced View Distance Framework (SE/AE), Phase 2 core

**Status: source complete for Phase 2, NOT yet compiled against CommonLibSSE-NG and NOT yet tested in game.**
The pure configuration logic (INI parsing, presets, clamping) is unit-tested (`tests/`, 35 checks, passing).
The engine-facing code was written without access to a Windows toolchain or the CommonLib headers, so the
first CI build may surface small API mismatches (each call site is commented). See "Known limits" below.

## What this build does

A native SKSE plugin that adjusts engine INI settings at runtime, as multipliers over the game's own values:

| Engine setting | Controls |
|---|---|
| `fLODFadeOutMultObjects:LOD` | static object fade distance |
| `fLODFadeOutMultItems:LOD` | item/clutter fade distance |
| `fLODFadeOutMultActors:LOD` | actor fade distance (only within loaded cells) |
| `fTreeLoadDistance:TerrainManager` | tree LOD load distance |
| `fBlockLevel0Distance:TerrainManager` | terrain LOD block range, level 0 |
| `fBlockLevel1Distance:TerrainManager` | terrain LOD block range, level 1 |

It installs **no rendering hooks and patches no memory**, so it does not touch Bottled Shaders' reverse-Z,
Hi-Z culling or shader pipeline. It logs runtime version, Community Shaders family detection and every value changed.

Presets (initial guesses, to be tuned): Low = no change; Balanced = 1.25-1.5x; High = 1.5-2x; Ultra = 2-3x.

## Requirements
- Skyrim SE/AE, SKSE64, [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444).
- Skyrim VR is not supported yet.
- Generated LOD (DynDOLOD / xLODGen / TexGen) is needed for distant objects and terrain. This plugin only changes how far existing LOD is shown. It cannot create LOD.

## Install (MO2 / Vortex)
Install as a mod so the files land at `SKSE/Plugins/AdvancedViewDistance.dll` and `SKSE/Plugins/AdvancedViewDistance.ini`.

## First test
1. Set `bDryRun=1` in the INI, launch, load a save, quit.
2. Open `Documents\My Games\Skyrim Special Edition\SKSE\AdvancedViewDistance.log` and confirm: runtime detected, settings found, intended values look sane.
3. Set `bDryRun=0`, relaunch, and compare. If anything looks wrong set `sPreset=Low` or `bEnabled=0`.
4. Send me the log (and your Skyrim version, Bottled Shaders version, whether DynDOLOD/xLODGen LOD is installed).

## Build from source (Windows)
Requires Visual Studio 2022 (C++), CMake 3.24+, vcpkg. See `.github/workflows/build.yml` for the exact steps, or just push this folder to GitHub and let the workflow build the DLL (artifact `AdvancedViewDistance`).
`vcpkg-configuration.json`: the Color-Glass registry baseline is a placeholder resolved by CI; pin it after the first good build.

Logic tests (any platform with g++):
```
g++ -std=c++20 -Wall -Wextra -Werror -I src tests/test_config.cpp -o test_config && ./test_config
```

## Compatibility notes (from Phase 1 research)
- Bottled Shaders is a Community Shaders fork; it is believed to ship as `CommunityShaders.dll`, so it cannot be distinguished from vanilla Community Shaders by name. Both get the same policy: no hooks.
- Its Hi-Z culling feeds from a `BSAccumProcess::RegisterObject` hook that is installed only on AE before 1.7.99 (Bottled Shaders PR #7). On SE, AE 1.7.99+ and VR that hook is not installed. A future culling hook from this project would therefore conflict only on AE <1.7.99, and is gated off regardless (`allowRenderingHooks`).

## Known limits
- Full models and actors exist only inside the loaded cell grid; distant cities/forests come from LOD assets.
- Too-high fade multipliers can make objects fade before the loaded-cell edge. Keep `fMaxMultiplier` conservative.
- Not yet verified in game: effect of each setting on SE vs AE, interaction with DynDOLOD's own settings, VR.
- No per-reference visibility policy, grass control, adaptive scaling or in-game UI yet (later phases).
