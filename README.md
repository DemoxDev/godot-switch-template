# Godot 4 export template for Nintendo Switch (homebrew)

An unofficial Godot **4.7.2** engine port for the Nintendo Switch homebrew stack
([devkitPro](https://devkitpro.org/) devkitA64 + [libnx](https://github.com/switchbrew/libnx) + Mesa/nouveau),
and the scripts to turn any Godot project into an installable `.nro`.

It is a small overlay on upstream Godot: a new `platform/switch/` directory plus three tiny patches.
Godot itself is downloaded at build time.

> **Status: experimental.** Built and tested with one real project on real hardware and in the
> [Eden](https://eden-emu.dev/) emulator. See [Limitations](#limitations) before relying on it.

## What works

Verified on a Switch running homebrew:

- Boot, `romfs` pack loading, Compatibility renderer (OpenGL ES 3.2 on Mesa 20.1 / nouveau), 2D canvas and text
- Audio (stereo, 48 kHz, through `audout`)
- Controller input (handheld + player 1)
- The system on-screen keyboard for `LineEdit` / `TextEdit`
- Loading and running a large (80 MB pack) project

Implemented but not verified: touchscreen input. Not verified: gameplay-heavy 3D scenes, performance, docked mode,
multiple controllers.

## Quick start

Requirements: Linux, Docker, git, and a Godot **4.7.2** editor binary (to export the pack).

```sh
scripts/build.sh                      # clones Godot 4.7.2, applies patches, builds out/godot.switch.template_release.arm64
scripts/export-nro.sh path/to/project --out MyGame.nro --author "Me"
```

Copy `MyGame.nro` to `sdmc:/switch/` on the SD card and start it from the Homebrew Menu.

`export-nro.sh` exports a pack with your editor (headless), then runs `pack-nro.sh`. Use `pack-nro.sh`
directly if you already have a `.pck`:

```sh
scripts/pack-nro.sh --pck game.pck --out MyGame.nro --name "My Game" --author "Me" --version 1.0.0 --icon icon.png
```

Project settings that matter:

- Use the **Compatibility** renderer (`rendering/renderer/rendering_method="gl_compatibility"`). The engine
  binary has no Vulkan.
- The export preset only provides a pack: any platform's preset works.
- Don't depend on modules that are disabled in [`platform/switch/detect.py`](platform/switch/detect.py)
  (Jolt Physics, glTF import at runtime, multiplayer/ENet/WebRTC/WebSocket, mbedTLS, advanced text server...).
  Re-enable one by passing it to the build: `scripts/build.sh module_gltf_enabled=yes`.

## How it fits together

```
platform/switch/    the port: OS, display server (EGL / GLES 3), audio, hid input, swkbd, POSIX shims
patches/            three small patches to upstream Godot (build wiring and a Horizon/newlib workaround each)
scripts/            setup (fetch + patch), build, pack-nro, export-nro
tools/gltest/       a small Godot project that adds 3D features one at a time and logs each step
docs/               porting notes: what broke, why, and how it was fixed
```

At runtime the NRO's romfs holds `game.pck`; the engine reports `romfs:/game.nro` as its executable so Godot
finds the pack the normal way. Per-game files live in `sdmc:/switch/<game name>/` (the `--name` you packed with,
with unsafe characters replaced by `_`; the NRO file name is the fallback):
`data/` (`user://`), `log.txt`, and an optional `args.txt` (engine arguments, one per line, e.g.
`--scene res://scenes/main.tscn`).

## Debugging

- Run `nxlink -s MyGame.nro` and the engine's output streams to your PC.
- With no `nxlink` host, output goes to `sdmc:/switch/<game name>/log.txt`, each line stamped with milliseconds since
  launch. Frames longer than 100 ms are logged as `PROFILE: slow frame N ms`.
- Create an empty file `sdmc:/switch/<game name>/gl_selftest` to log GPU info and a texture-format self test.
- [`tools/gltest`](tools/gltest) isolates which 3D feature breaks on a given setup.
- Eden works for quick iterations, but its GPU emulation is not the console's: it logs shader-translation errors
  the real GPU doesn't have, and its timings are not representative.

## Limitations

- **First load is slow.** Godot compiles its shaders at runtime. The Switch's Mesa returns empty program binaries
  (Godot's shader cache stores nothing) and has no disk cache, so every launch recompiles. Expect a long first
  black screen on big projects; fewer material/shader variants help.
- Fixed 1280x720 output (docked mode is scaled by the system).
- No GDExtension (no `dlopen`), no process spawning, no network interface listing, no vibration or motion sensors.
- Saves go to the SD card, not Switch save data.
- Only `template_release` is tested.
- Godot reports no worker threads on Horizon, so its thread-pool tasks run on the calling thread.
- Physics: Godot Physics only (Jolt has no Horizon platform support).

## Legal

Unofficial homebrew. Contains no Nintendo SDK code or proprietary assets and is not affiliated with Nintendo,
the Godot Engine, or devkitPro. To release a commercial Switch title you need Nintendo's official developer
program; community homebrew builds don't replace that. See [NOTICE](NOTICE) and [LICENSE](LICENSE).

## Credits

Godot Engine and its contributors. The earlier community Godot 3 Switch ports
([Stary2001/godot](https://github.com/Stary2001/godot), [fhidalgosola/godot-switch](https://github.com/fhidalgosola/godot-switch))
showed the libnx + EGL approach; this port is a fresh implementation for Godot 4.
