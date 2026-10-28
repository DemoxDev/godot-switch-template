#include "os_switch.h"

#include "main/main.h"

#include "nx_wrap.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

int main(int argc, char *argv[]) {
	nx::init(argv[0]);

	OS_Switch os;

	// No CLI on console: the pack ships in romfs as game.pck (see OS_Switch::get_executable_path).
	static char arg_display_flag[] = "--display-driver";
	static char arg_display[] = "switch";
	static char arg_render_flag[] = "--rendering-driver";
	static char arg_render[] = "opengl3";
	static char arg_method_flag[] = "--rendering-method";
	static char arg_method[] = "gl_compatibility";
	static char arg_verbose[] = "--verbose";
	std::vector<char *> args = { arg_verbose, arg_display_flag, arg_display, arg_render_flag, arg_render, arg_method_flag, arg_method };

	// Developer escape hatch: extra engine arguments, one per line, from <app dir>/args.txt on the SD card
	// (e.g. `--scene res://scenes/run/run_controller.tscn` to skip the menus).
	static std::vector<std::string> extra;
	const std::string args_path = std::string(nx::app_dir()) + "/args.txt";
	if (FILE *f = fopen(args_path.c_str(), "r")) {
		char line[512];
		while (fgets(line, sizeof(line), f)) {
			std::string s(line);
			while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ')) {
				s.pop_back();
			}
			if (!s.empty()) {
				extra.push_back(s);
			}
		}
		fclose(f);
	}
	for (std::string &s : extra) {
		args.push_back(&s[0]);
	}

	Error err = Main::setup(argv[0], (int)args.size(), args.data());
	int exit_code = EXIT_SUCCESS;

	if (err != OK) {
		exit_code = (err == ERR_HELP) ? EXIT_SUCCESS : EXIT_FAILURE;
	} else if (Main::start() == EXIT_SUCCESS) {
		os.run();
		exit_code = os.get_exit_code();
	} else {
		exit_code = EXIT_FAILURE;
	}
	Main::cleanup();

	nx::exit();
	return exit_code;
}
