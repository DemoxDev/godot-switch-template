#pragma once

// Plain-C facade over libnx. libnx's <switch.h> defines `Mutex` and `Thread`, which collide with
// Godot's classes, so no Godot translation unit may include it: everything console-specific
// goes through here instead.

#include <cstddef>
#include <cstdint>

namespace nx {
// romfs, sockets, nxlink stdio (or an SD-card log), and the per-app SD folder. `p_argv0` is the NRO path
// hbmenu passes in. The per-app folder is `sdmc:/switch/<id>`, where <id> comes from romfs:/app.id (written by
// pack-nro.sh), else the NRO file name without .nro, else "godot-app".
void init(const char *p_argv0);
void exit();
bool applet_loop(); // false once the OS asks the app to quit.
// Per-app SD folder, e.g. `sdmc:/switch/MyGame`: config, data, cache, log.txt and args.txt live here.
const char *app_dir();
bool file_exists(const char *p_path);
bool firmware_version(char *r_buf, size_t p_len);

// The EGL native window, fixed at 1280x720 so docked mode is scaled by the compositor
// instead of changing the framebuffer size under the engine.
void *default_window();

// ---- Input ----
enum PadButton : uint32_t {
	PAD_A = 1u << 0,
	PAD_B = 1u << 1,
	PAD_X = 1u << 2,
	PAD_Y = 1u << 3,
	PAD_L = 1u << 4,
	PAD_R = 1u << 5,
	PAD_ZL = 1u << 6,
	PAD_ZR = 1u << 7,
	PAD_PLUS = 1u << 8,
	PAD_MINUS = 1u << 9,
	PAD_UP = 1u << 10,
	PAD_DOWN = 1u << 11,
	PAD_LEFT = 1u << 12,
	PAD_RIGHT = 1u << 13,
	PAD_LSTICK = 1u << 14,
	PAD_RSTICK = 1u << 15,
};

struct PadSnapshot {
	bool connected = false;
	uint32_t buttons = 0; // PadButton bits, held this frame.
	int16_t lx = 0, ly = 0, rx = 0, ry = 0; // -32767..32767, +y is up.
};

struct TouchPoint {
	uint32_t id = 0;
	int x = 0;
	int y = 0;
};

void input_init();
PadSnapshot pad_poll(); // Player 1 and handheld, merged.
int touch_poll(TouchPoint *r_points, int p_max); // Returns the number of fingers down.

// ---- Audio (audout: 48 kHz, 2 ch, s16) ----
constexpr uint32_t AUDIO_FRAMES_PER_BUFFER = 1024;

bool audio_init();
void audio_exit();
uint32_t audio_sample_rate();
// Queues exactly AUDIO_FRAMES_PER_BUFFER interleaved stereo frames, blocking until a buffer is free,
// so the hardware clock paces the caller.
void audio_submit(const int16_t *p_samples);

// Diagnostic: three magenta frames straight through Mesa GLES + EGL, before Godot's renderer takes
// over. If the screen never shows magenta, frames are not reaching the display at all.
void present_test(void *p_egl_display, void *p_egl_surface, int p_w, int p_h);

// Startup diagnostic: draws known textures (RGBA8, RG8 + swizzle as Godot's text path uses, R8) through
// a minimal GLES3 shader, reads the pixel back and logs expected vs. actual. Needs a current context.
void gl_selftest();

// System software keyboard (blocks while the applet is up). Returns false if the user cancelled.
bool keyboard_show(const char *p_initial, const char *p_guide, int p_max_len, char *r_out, size_t p_out_size);

// Milliseconds on the system tick counter (monotonic, arbitrary origin).
uint64_t millis();
} // namespace nx
