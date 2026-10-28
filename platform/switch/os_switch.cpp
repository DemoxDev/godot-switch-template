#include "os_switch.h"

#include "core/config/project_settings.h"
#include "core/input/input.h"
#include "core/os/main_loop.h"
#include "main/main.h"
#include "display_server_switch.h"
#include "servers/display/display_server.h"

#include "nx_wrap.h"

#include <cstdio>

void OS_Switch::initialize() {
	OS_Unix::initialize_core();
}

void OS_Switch::initialize_joypads() {
	nx::input_init();
}

// Nintendo labels map to Godot's: Switch A (east) is the accept button, B (south) cancels, as on every
// other Switch title, which is the opposite of Godot's positional A/B.
void OS_Switch::_process_pad() {
	nx::PadSnapshot s = nx::pad_poll();
	Input *input = Input::get_singleton();

	if (s.connected != pad_connected) {
		pad_connected = s.connected;
		input->joy_connection_changed(0, s.connected, "Nintendo Switch Controller", "switch-pad");
		pad_prev_buttons = 0;
	}
	if (!s.connected) {
		return;
	}

	static const struct {
		uint32_t bit;
		JoyButton button;
	} buttons[] = {
		{ nx::PAD_A, JoyButton::A },
		{ nx::PAD_B, JoyButton::B },
		{ nx::PAD_X, JoyButton::X },
		{ nx::PAD_Y, JoyButton::Y },
		{ nx::PAD_L, JoyButton::LEFT_SHOULDER },
		{ nx::PAD_R, JoyButton::RIGHT_SHOULDER },
		{ nx::PAD_PLUS, JoyButton::START },
		{ nx::PAD_MINUS, JoyButton::BACK },
		{ nx::PAD_UP, JoyButton::DPAD_UP },
		{ nx::PAD_DOWN, JoyButton::DPAD_DOWN },
		{ nx::PAD_LEFT, JoyButton::DPAD_LEFT },
		{ nx::PAD_RIGHT, JoyButton::DPAD_RIGHT },
		{ nx::PAD_LSTICK, JoyButton::LEFT_STICK },
		{ nx::PAD_RSTICK, JoyButton::RIGHT_STICK },
	};

	uint32_t changed = s.buttons ^ pad_prev_buttons;
	for (const auto &b : buttons) {
		if (changed & b.bit) {
			input->joy_button(0, b.button, (s.buttons & b.bit) != 0);
		}
	}
	if (changed & nx::PAD_ZL) {
		input->joy_axis(0, JoyAxis::TRIGGER_LEFT, (s.buttons & nx::PAD_ZL) ? 1.0f : 0.0f);
	}
	if (changed & nx::PAD_ZR) {
		input->joy_axis(0, JoyAxis::TRIGGER_RIGHT, (s.buttons & nx::PAD_ZR) ? 1.0f : 0.0f);
	}
	pad_prev_buttons = s.buttons;

	// libnx reports +y up; Godot wants +y down.
	input->joy_axis(0, JoyAxis::LEFT_X, s.lx / 32767.0f);
	input->joy_axis(0, JoyAxis::LEFT_Y, -s.ly / 32767.0f);
	input->joy_axis(0, JoyAxis::RIGHT_X, s.rx / 32767.0f);
	input->joy_axis(0, JoyAxis::RIGHT_Y, -s.ry / 32767.0f);
}

void OS_Switch::finalize() {
	if (main_loop) {
		memdelete(main_loop);
	}
	main_loop = nullptr;
}

MainLoop *OS_Switch::get_main_loop() const {
	return main_loop;
}

void OS_Switch::delete_main_loop() {
	if (main_loop) {
		memdelete(main_loop);
	}
	main_loop = nullptr;
}

void OS_Switch::set_main_loop(MainLoop *p_main_loop) {
	main_loop = p_main_loop;
}

String OS_Switch::get_identifier() const {
	return "switch";
}

String OS_Switch::get_name() const {
	return "Switch";
}

// Godot looks for `<executable basename>.pck`, so the pack baked into the NRO's romfs is `game.pck`
// and the "executable" is reported as `romfs:/game.nro`. This works with or without path overrides.
String OS_Switch::get_executable_path() const {
	return "romfs:/game.nro";
}

String OS_Switch::get_distribution_name() const {
	return "Horizon";
}

String OS_Switch::get_version() const {
	char buf[32];
	return nx::firmware_version(buf, sizeof(buf)) ? String(buf) : String();
}

// Saves live in the per-app SD folder (named after the NRO), so they survive reinstalling the game.
String OS_Switch::get_config_path() const {
	return String(nx::app_dir()) + "/config";
}

String OS_Switch::get_data_path() const {
	return String(nx::app_dir()) + "/data";
}

String OS_Switch::get_cache_path() const {
	return String(nx::app_dir()) + "/cache";
}

String OS_Switch::get_unique_id() const {
	return "switch";
}

String OS_Switch::get_processor_name() const {
	return "NVIDIA Tegra X1 (Cortex-A57)";
}

bool OS_Switch::_check_internal_feature_support(const String &p_feature) {
	return p_feature == "mobile" || p_feature == "switch" || p_feature == "arm64";
}

void OS_Switch::run() {
	if (!main_loop) {
		return;
	}

	main_loop->initialize();

	while (nx::applet_loop()) {
		_process_pad();
		DisplayServer::get_singleton()->process_events();
		if (Main::iteration()) {
			break;
		}
	}

	main_loop->finalize();
}

OS_Switch::OS_Switch() {
	main_loop = nullptr;

	AudioDriverManager::add_driver(&driver_switch_audio);
	DisplayServerSwitch::register_switch_driver();
}
