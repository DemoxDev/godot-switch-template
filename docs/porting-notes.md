# Porting notes

What had to change to get Godot 4.7.2 running on Horizon (the Switch OS) with devkitPro, and why. Useful
when moving to a newer Godot version or porting to a similar newlib-based target.

## Design

- `OS_Switch` extends `OS_Unix`. `DisplayServerSwitch` extends `DisplayServerHeadless` and overrides only what a
  console does (see "Headless base class traps").
- Graphics: Mesa's nouveau EGL, OpenGL ES 3 context, Godot's Compatibility renderer. Godot's own
  `drivers/egl/egl_manager` is not built: it needs `eglGetPlatformDisplay` and glad's `dlopen`-based EGL loader.
  The display server calls `eglGetDisplay(EGL_DEFAULT_DISPLAY)` and `eglCreateWindowSurface` on libnx's default
  `NWindow` (set to 1280x720). glad's GL loader is still used, bound to Mesa's `eglGetProcAddress` through
  `EGL_STATIC` (patch 0001).
- Audio: `audout`, 48 kHz stereo s16, four 1024-frame buffers. `nx::audio_submit` blocks until a buffer is free,
  so the hardware paces the mix thread.
- Input: `hid` pad state polled each frame and mapped to Godot joypad events; touch becomes
  `InputEventScreenTouch`/`Drag`. Switch A/B/X/Y map to Godot A/B/X/Y *by label* (A confirms), not by position.
- Keyboard: `swkbd` (a blocking applet). See "Keyboard".

## Things that broke, and the fix

### libnx's `Mutex` and `Thread` collide with Godot's
`<switch.h>` defines both. Including it in any Godot translation unit fails to compile. All libnx calls live
in `platform/switch/nx_wrap.{h,cpp}`, a plain-C++ facade; **no Godot file may include `<switch.h>`**.

### newlib's `_S` macro
newlib's `<ctype.h>` defines `_U _L _N _S _P _C _X _B`. Manifold's SVD code has a variable named `_S`. Undefining
the macros breaks newlib's own inline `isalpha` and friends, so patch 0003 renames the variable.

### Jolt Physics has no Horizon platform
`Jolt/Core/Core.h` ends in `#error Unknown platform`. The module is disabled (`module_jolt_physics_enabled=no`).

### Missing POSIX, shimmed
Horizon's newlib declares but doesn't link the process and signal APIs, and lacks some headers:

- `posix_stubs.cpp`: `popen`, `pclose`, `pipe`, `waitpid`, `setsid`, `sigaction`, `mkfifo`, `execvp` fail with `ENOSYS`.
- `compat/`: `sys/un.h`, `syslog.h`, `ifaddrs.h`, `dlfcn.h`, `pthread_np.h`, and a force-included
  `switch_compat.h` for `htons`/`ntohs` (from `<arpa/inet.h>`), `getrlimit`, `if_nametoindex`, `ipv6_mreq`.
- `file_access_unix.cpp` has no `<sys/xattr.h>`: patch 0002 adds the Switch to the existing "not supported" branches.

### Release templates have no `--main-pack`
`disable_path_overrides` defaults to yes for `template_release`, and Godot aborts at startup if the flag is
used. The port instead overrides `get_executable_path()` to `romfs:/game.nro`, so Godot's normal
"`<exe basename>.pck` next to the executable" lookup finds `romfs:/game.pck`. The template also builds with
`disable_path_overrides=no` so developers can use `--scene` from `args.txt`.

### Headless base class traps
`DisplayServerHeadless` hardcodes `window_can_draw()` and `can_any_window_draw()` to `false` and returns an
empty `get_window_list()`. `Main::iteration()` then never presents a frame: you see the boot splash (drawn
explicitly) and nothing after, while audio and input keep working. The Switch display server overrides all three.

### The keyboard
`LineEdit` asks for the virtual keyboard the moment the field gains focus, during scene setup, before the first
frame is presented. `swkbd` blocks the app, so it opened over a black screen. The request is held for 45 presented
frames and dropped if the field loses focus first. The result reaches the field as key events (Ctrl+A to select all,
then characters or Backspace) on the next `process_events()`, never from inside `LineEdit`'s focus handling.

### Shader compilation is slow and uncacheable
- Godot's GLES3 program-binary cache writes 40-88 byte files (a `GLSC` header, empty payload): Mesa returns
  zero-length binaries here.
- Mesa's own disk cache (`MESA_GLSL_CACHE_DIR`) is compiled out of the devkitPro `switch-mesa` build (no env var
  strings in `libEGL.a`).
- So every launch recompiles. In Eden the big scene shader alone stalled startup for ~6 s, with 3.4 s and 1.7 s
  stalls on the first 3D frames; a Switch's CPU is much slower than the desktop running Eden.

## Debugging playbook

1. **Get logs off the console**: `nxlink -s`, or the SD log. stdout is redirected through a stream that timestamps
   lines and flushes per line, so the last thing printed survives a hang.
2. **Reproduce in Eden** with `args.txt` (`--scene res://...`) to skip menus, and read the SD log from Eden's
   virtual SD (`~/.local/share/eden/sdmc/switch/<game name>/log.txt`).
3. **Bisect the GPU path** with `tools/gltest`: it adds a feature per stage (3D, lights, normal map, sky/fog, custom
   shader, shadows, SubViewport, glow, screen-texture shader) and logs `STAGE n: applying` / `OK`.
4. **Check texture formats** with the opt-in `gl_selftest` flag file: it draws RGBA8, immutable-storage, RG8-swizzle
   (Godot's text path), R8 and mipmapped textures and logs expected vs. actual pixels.
5. Beware: a long black screen is usually shader compilation, not a hang. Compare timestamps before assuming.

## Emulator caveats (Eden)

- Logs `Instruction IMAD (reg) is not implemented` for some shaders and asserts in `maxwell_dma`.
- Reports `Game: 0 FPS` while the guest is busy compiling shaders.
- Runs the real guest Mesa, so GL behavior (formats, extensions, cache support) matches the console; GPU and CPU
  speed do not.

## Updating to another Godot version

1. Change `GODOT_VERSION`.
2. `scripts/setup.sh` (use a fresh `build/`); fix any patch that no longer applies. Each patch is one concern.
3. Expect to revisit: `DisplayServer`/`OS` virtuals (new pure virtuals fail to compile), `platform_*.h` headers,
   the glad loader wiring, and new thirdparty code that assumes glibc or a known platform.
