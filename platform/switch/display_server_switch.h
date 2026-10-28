#pragma once

#include "core/templates/hash_map.h"
#include "servers/display/display_server_headless.h"

#include <EGL/egl.h>

/// A single fixed 1280x720 window backed by Mesa/nouveau EGL (OpenGL ES 3). Inherits the headless
/// no-ops for every desktop-only call (window modes, cursors, clipboard, dialogs...), and overrides
/// only what a console actually does: report the screen, present frames and turn touches into events.
class DisplayServerSwitch : public DisplayServerHeadless {
	GDSOFTCLASS(DisplayServerSwitch, DisplayServerHeadless);

	static constexpr int SCREEN_W = 1280;
	static constexpr int SCREEN_H = 720;

	EGLDisplay egl_display = EGL_NO_DISPLAY;
	EGLSurface egl_surface = EGL_NO_SURFACE;
	EGLContext egl_context = EGL_NO_CONTEXT;

	String rendering_driver;
	DisplayServerEnums::VSyncMode vsync_mode = DisplayServerEnums::VSYNC_ENABLED;

	// LineEdit asks for the keyboard the moment it gains focus, i.e. during scene setup and before the
	// first frame is presented. The system keyboard is a blocking applet, so it would open over a black
	// screen. The request is held for a few frames so the prompt is visible first.
	static constexpr int KEYBOARD_DELAY_FRAMES = 45;
	bool keyboard_requested = false;
	int keyboard_wait_frames = 0;
	String keyboard_existing_text;
	int keyboard_max_length = 500;

	String pending_keyboard_text;
	bool has_pending_keyboard_text = false;

	void _inject_keyboard_text(const String &p_text);

	HashMap<uint32_t, Vector2> touches; // finger id -> last position.

	Error _init_egl();
	void _destroy_egl();

	static DisplayServer *create_func(const String &p_rendering_driver, DisplayServerEnums::WindowMode p_mode, DisplayServerEnums::VSyncMode p_vsync_mode, uint32_t p_flags, const Vector2i *p_position, const Vector2i &p_resolution, int p_screen, DisplayServerEnums::Context p_context, int64_t p_parent_window, Error &r_error);
	static Vector<String> get_rendering_drivers_func();

public:
	static void register_switch_driver();

	String get_name() const override { return "switch"; }
	bool has_feature(DisplayServerEnums::Feature p_feature) const override;

	int get_screen_count() const override { return 1; }
	int get_primary_screen() const override { return 0; }
	Point2i screen_get_position(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override { return Point2i(); }
	Size2i screen_get_size(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override { return Size2i(SCREEN_W, SCREEN_H); }
	Rect2i screen_get_usable_rect(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override { return Rect2i(0, 0, SCREEN_W, SCREEN_H); }
	int screen_get_dpi(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override { return 237; }
	float screen_get_refresh_rate(int p_screen = DisplayServerEnums::SCREEN_OF_MAIN_WINDOW) const override { return 60.0f; }

	// The headless base reports no windows and "can't draw", so Main::iteration() would never present a
	// frame after the boot splash. We always have exactly one drawable window.
	Vector<DisplayServerEnums::WindowID> get_window_list() const override {
		Vector<DisplayServerEnums::WindowID> list;
		list.push_back(DisplayServerEnums::MAIN_WINDOW_ID);
		return list;
	}
	bool window_can_draw(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override { return true; }
	bool can_any_window_draw() const override { return true; }

	int window_get_current_screen(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override { return 0; }
	Size2i window_get_size(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override { return Size2i(SCREEN_W, SCREEN_H); }
	Size2i window_get_size_with_decorations(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override { return Size2i(SCREEN_W, SCREEN_H); }

	void window_set_vsync_mode(DisplayServerEnums::VSyncMode p_vsync_mode, DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) override;
	DisplayServerEnums::VSyncMode window_get_vsync_mode(DisplayServerEnums::WindowID p_window = DisplayServerEnums::MAIN_WINDOW_ID) const override { return vsync_mode; }

	void virtual_keyboard_show(const String &p_existing_text, const Rect2 &p_screen_rect = Rect2(), DisplayServerEnums::VirtualKeyboardType p_type = DisplayServerEnums::KEYBOARD_TYPE_DEFAULT, int p_max_length = -1, int p_cursor_start = -1, int p_cursor_end = -1) override;

	void virtual_keyboard_hide() override;

	void process_events() override;
	void swap_buffers() override;

	DisplayServerSwitch(const String &p_rendering_driver, DisplayServerEnums::VSyncMode p_vsync_mode, Error &r_error);
	~DisplayServerSwitch();
};
