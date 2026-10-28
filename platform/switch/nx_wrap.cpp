#include "nx_wrap.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <switch.h>

#include <ctype.h>
#include <strings.h>
#include <sys/stat.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace nx {

uint64_t millis() {
	return armTicksToNs(armGetSystemTick()) / 1000000ULL;
}

static FILE *log_file = nullptr;
static uint64_t log_start_ms = 0;
static bool log_at_line_start = true;

static int _log_write(void *, const char *p_buf, size_t p_len) {
	if (!log_file) {
		return (int)p_len;
	}
	for (size_t i = 0; i < p_len; i++) {
		if (log_at_line_start) {
			uint64_t t = millis() - log_start_ms;
			fprintf(log_file, "[%4llu.%03llu] ", (unsigned long long)(t / 1000), (unsigned long long)(t % 1000));
			log_at_line_start = false;
		}
		fputc(p_buf[i], log_file);
		if (p_buf[i] == '\n') {
			log_at_line_start = true;
		}
	}
	fflush(log_file);
	return (int)p_len;
}

static char g_app_dir[256] = "sdmc:/switch/godot-app";

// Folder name sources, in order: `romfs:/app.id` (written by scripts/pack-nro.sh from the game name, so it
// survives renaming the .nro and works where no path is passed, e.g. emulators), then the NRO file name,
// then "godot-app".
static void _use_folder_name(char *p_name) {
	for (char *p = p_name; *p; p++) {
		if (!(isalnum((unsigned char)*p) || *p == '.' || *p == '-' || *p == '_')) {
			*p = '_';
		}
	}
	if (p_name[0] && p_name[0] != '.') {
		snprintf(g_app_dir, sizeof(g_app_dir), "sdmc:/switch/%s", p_name);
	}
}

static bool _read_app_id() {
	FILE *f = fopen("romfs:/app.id", "r");
	if (!f) {
		return false;
	}
	char id[128] = {};
	bool ok = fgets(id, sizeof(id), f) != nullptr;
	fclose(f);
	if (!ok) {
		return false;
	}
	size_t n = strlen(id);
	while (n > 0 && (id[n - 1] == '\n' || id[n - 1] == '\r' || id[n - 1] == ' ')) {
		id[--n] = '\0';
	}
	if (!id[0]) {
		return false;
	}
	_use_folder_name(id);
	return true;
}

static void _derive_app_dir(const char *p_argv0) {
	if (_read_app_id()) {
		return;
	}
	if (!p_argv0 || !*p_argv0) {
		return;
	}
	const char *base = strrchr(p_argv0, '/');
	base = base ? base + 1 : p_argv0;
	char stem[128];
	size_t n = 0;
	for (; base[n] && n < sizeof(stem) - 1; n++) {
		stem[n] = base[n];
	}
	stem[n] = '\0';
	if (n >= 4 && strcasecmp(stem + n - 4, ".nro") == 0) {
		stem[n - 4] = '\0';
	}
	_use_folder_name(stem);
}

const char *app_dir() {
	return g_app_dir;
}

bool file_exists(const char *p_path) {
	struct stat st;
	return stat(p_path, &st) == 0;
}

void init(const char *p_argv0) {
	romfsInit();
	socketInitializeDefault();

	_derive_app_dir(p_argv0);
	mkdir("sdmc:/switch", 0777);
	mkdir(g_app_dir, 0777);

	// Mirror stdout/stderr to `nxlink -s` on the PC; with no host listening (emulator, or a console
	// with no PC attached) fall back to a log on the SD card, with every line stamped with the
	// milliseconds since launch so the log shows where load time goes.
	if (nxlinkStdio() < 0) {
		char log_path[300];
		snprintf(log_path, sizeof(log_path), "%s/log.txt", g_app_dir);
		log_file = fopen(log_path, "w");
		if (log_file) {
			log_start_ms = millis();
			if (FILE *timed = funopen(nullptr, nullptr, _log_write, nullptr, nullptr)) {
				stdout = timed;
				setvbuf(stdout, nullptr, _IOLBF, 0);
				stderr = stdout;
			}
		}
	}
}

void exit() {
	socketExit();
	romfsExit();
}

bool applet_loop() {
	return appletMainLoop();
}

bool firmware_version(char *r_buf, size_t p_len) {
	SetSysFirmwareVersion fw;
	if (R_FAILED(setsysInitialize())) {
		return false;
	}
	Result rc = setsysGetFirmwareVersion(&fw);
	setsysExit();
	if (R_FAILED(rc)) {
		return false;
	}
	strncpy(r_buf, fw.display_version, p_len - 1);
	r_buf[p_len - 1] = '\0';
	return true;
}

void *default_window() {
	NWindow *win = nwindowGetDefault();
	nwindowSetDimensions(win, 1280, 720);
	return win;
}

// ---- Input ----

static PadState pad;

void input_init() {
	padConfigureInput(1, HidNpadStyleSet_NpadStandard);
	padInitializeDefault(&pad);
	hidInitializeTouchScreen();
}

PadSnapshot pad_poll() {
	padUpdate(&pad);

	static const struct {
		uint64_t hid;
		uint32_t mine;
	} map[] = {
		{ HidNpadButton_A, PAD_A },
		{ HidNpadButton_B, PAD_B },
		{ HidNpadButton_X, PAD_X },
		{ HidNpadButton_Y, PAD_Y },
		{ HidNpadButton_L, PAD_L },
		{ HidNpadButton_R, PAD_R },
		{ HidNpadButton_ZL, PAD_ZL },
		{ HidNpadButton_ZR, PAD_ZR },
		{ HidNpadButton_Plus, PAD_PLUS },
		{ HidNpadButton_Minus, PAD_MINUS },
		{ HidNpadButton_Up, PAD_UP },
		{ HidNpadButton_Down, PAD_DOWN },
		{ HidNpadButton_Left, PAD_LEFT },
		{ HidNpadButton_Right, PAD_RIGHT },
		{ HidNpadButton_StickL, PAD_LSTICK },
		{ HidNpadButton_StickR, PAD_RSTICK },
	};

	PadSnapshot s;
	s.connected = padIsConnected(&pad);
	uint64_t held = padGetButtons(&pad);
	for (const auto &m : map) {
		if (held & m.hid) {
			s.buttons |= m.mine;
		}
	}
	HidAnalogStickState l = padGetStickPos(&pad, 0);
	HidAnalogStickState r = padGetStickPos(&pad, 1);
	s.lx = (int16_t)l.x;
	s.ly = (int16_t)l.y;
	s.rx = (int16_t)r.x;
	s.ry = (int16_t)r.y;
	return s;
}

int touch_poll(TouchPoint *r_points, int p_max) {
	HidTouchScreenState state = {};
	if (hidGetTouchScreenStates(&state, 1) == 0) {
		return 0;
	}
	int n = state.count < p_max ? state.count : p_max;
	for (int i = 0; i < n; i++) {
		r_points[i].id = state.touches[i].finger_id;
		r_points[i].x = (int)state.touches[i].x;
		r_points[i].y = (int)state.touches[i].y;
	}
	return n;
}

// ---- Audio ----

static constexpr int BUF_COUNT = 4;
static constexpr uint32_t BUF_BYTES = AUDIO_FRAMES_PER_BUFFER * 2 * sizeof(int16_t); // 4096 = one page, as audout requires.

static AudioOutBuffer out_bufs[BUF_COUNT];
static void *out_data[BUF_COUNT];
static bool out_free[BUF_COUNT];
static bool audio_ready = false;

bool audio_init() {
	if (R_FAILED(audoutInitialize())) {
		return false;
	}
	for (int i = 0; i < BUF_COUNT; i++) {
		out_data[i] = aligned_alloc(0x1000, BUF_BYTES);
		memset(out_data[i], 0, BUF_BYTES);
		out_bufs[i] = {};
		out_bufs[i].buffer = out_data[i];
		out_bufs[i].buffer_size = BUF_BYTES;
		out_bufs[i].data_size = BUF_BYTES;
		out_free[i] = true;
	}
	if (R_FAILED(audoutStartAudioOut())) {
		audoutExit();
		return false;
	}
	audio_ready = true;
	return true;
}

void audio_exit() {
	if (!audio_ready) {
		return;
	}
	audio_ready = false;
	audoutStopAudioOut();
	audoutExit();
	for (int i = 0; i < BUF_COUNT; i++) {
		free(out_data[i]);
		out_data[i] = nullptr;
	}
}

uint32_t audio_sample_rate() {
	return audoutGetSampleRate();
}

static void _mark_released(AudioOutBuffer *p_buf) {
	for (int i = 0; i < BUF_COUNT; i++) {
		if (p_buf && p_buf->buffer == out_data[i]) {
			out_free[i] = true;
		}
	}
}

void audio_submit(const int16_t *p_samples) {
	if (!audio_ready) {
		return;
	}

	// Reclaim anything the hardware already finished, without blocking.
	AudioOutBuffer *released = nullptr;
	u32 count = 0;
	while (R_SUCCEEDED(audoutGetReleasedAudioOutBuffer(&released, &count)) && count > 0) {
		_mark_released(released);
		count = 0;
	}

	int idx = -1;
	for (int i = 0; i < BUF_COUNT; i++) {
		if (out_free[i]) {
			idx = i;
			break;
		}
	}
	while (idx < 0) { // All queued: sleep until the hardware frees one.
		if (R_FAILED(audoutWaitPlayFinish(&released, &count, UINT64_MAX))) {
			return;
		}
		_mark_released(released);
		for (int i = 0; i < BUF_COUNT; i++) {
			if (out_free[i]) {
				idx = i;
				break;
			}
		}
	}

	memcpy(out_data[idx], p_samples, BUF_BYTES);
	out_free[idx] = false;
	audoutAppendAudioOutBuffer(&out_bufs[idx]);
}

void present_test(void *p_egl_display, void *p_egl_surface, int p_w, int p_h) {
	EGLDisplay dpy = (EGLDisplay)p_egl_display;
	EGLSurface surf = (EGLSurface)p_egl_surface;
	printf("Switch GL: %s | %s\n", (const char *)glGetString(GL_RENDERER), (const char *)glGetString(GL_VERSION));
	for (int i = 0; i < 3; i++) {
		glViewport(0, 0, p_w, p_h);
		glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		EGLBoolean ok = eglSwapBuffers(dpy, surf);
		printf("Switch GL: magenta frame %d swap=%d egl_err=0x%x gl_err=0x%x\n", i, (int)ok, eglGetError(), glGetError());
	}
}

// ---- GL self-test ----

static GLuint _compile(GLenum p_type, const char *p_src) {
	GLuint s = glCreateShader(p_type);
	glShaderSource(s, 1, &p_src, nullptr);
	glCompileShader(s);
	GLint ok = 0;
	glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
	if (!ok) {
		char log[512];
		glGetShaderInfoLog(s, sizeof(log), nullptr, log);
		printf("GLTEST: shader compile failed: %s\n", log);
	}
	return s;
}

// Draws the bound texture over the whole 64x64 viewport and returns the centre pixel.
static void _draw_and_read(GLuint p_prog, GLuint p_tex, const char *p_label, const unsigned char *p_expected) {
	glViewport(0, 0, 64, 64);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(p_prog);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, p_tex);
	glUniform1i(glGetUniformLocation(p_prog, "t"), 0);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	unsigned char px[4] = { 0, 0, 0, 0 };
	glReadPixels(32, 32, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px);
	GLenum err = glGetError();
	bool match = true;
	for (int i = 0; i < 4; i++) {
		int d = (int)px[i] - (int)p_expected[i];
		if (d < -2 || d > 2) {
			match = false;
		}
	}
	printf("GLTEST: %-34s got=(%3u,%3u,%3u,%3u) want=(%3u,%3u,%3u,%3u) glerr=0x%x %s\n", p_label, px[0], px[1], px[2], px[3], p_expected[0], p_expected[1], p_expected[2], p_expected[3], err, match ? "OK" : "MISMATCH");
}

void gl_selftest() {
	char flag[300];
	snprintf(flag, sizeof(flag), "%s/gl_selftest", g_app_dir);
	if (!file_exists(flag)) {
		return; // Opt-in: create an empty `gl_selftest` file in the app folder.
	}
	printf("GLTEST: renderer=%s | version=%s | glsl=%s\n", (const char *)glGetString(GL_RENDERER), (const char *)glGetString(GL_VERSION), (const char *)glGetString(GL_SHADING_LANGUAGE_VERSION));
	const char *ext = (const char *)glGetString(GL_EXTENSIONS);
	printf("GLTEST: extensions=%s\n", ext ? ext : "(null)");

	GLuint vs = _compile(GL_VERTEX_SHADER, "#version 300 es\nout vec2 uv; void main(){ vec2 p=vec2((gl_VertexID<<1)&2, gl_VertexID&2); uv=p; gl_Position=vec4(p*2.0-1.0,0.0,1.0);} ");
	GLuint fs = _compile(GL_FRAGMENT_SHADER, "#version 300 es\nprecision highp float; in vec2 uv; uniform sampler2D t; out vec4 c; void main(){ c=texture(t, uv);} ");
	GLuint prog = glCreateProgram();
	glAttachShader(prog, vs);
	glAttachShader(prog, fs);
	glLinkProgram(prog);
	GLint linked = 0;
	glGetProgramiv(prog, GL_LINK_STATUS, &linked);
	printf("GLTEST: program link=%d\n", linked);
	if (!linked) {
		return;
	}

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint tex;
	glGenTextures(1, &tex);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// 1. Plain RGBA8, mutable storage, LINEAR no mips (how most UI art is sampled).
	{
		unsigned char data[4 * 4] = { 255, 0, 255, 255, 255, 0, 255, 255, 255, 0, 255, 255, 255, 0, 255, 255 };
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		const unsigned char want[4] = { 255, 0, 255, 255 };
		_draw_and_read(prog, tex, "RGBA8 glTexImage2D", want);
	}

	// 2. Same, but immutable storage + sub-image upload, as Godot's texture_2d_initialize does.
	{
		GLuint t2;
		glGenTextures(1, &t2);
		glBindTexture(GL_TEXTURE_2D, t2);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 2, 2);
		unsigned char data[4 * 4] = { 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255 };
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 2, 2, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		const unsigned char want[4] = { 0, 255, 0, 255 };
		_draw_and_read(prog, t2, "RGBA8 glTexStorage2D+SubImage", want);
		glDeleteTextures(1, &t2);
	}

	// 3. RG8 with the (R,R,R,G) swizzle Godot uses for LA8 glyph atlases (text!).
	{
		GLuint t3;
		glGenTextures(1, &t3);
		glBindTexture(GL_TEXTURE_2D, t3);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_RG8, 2, 2);
		unsigned char data[2 * 4] = { 200, 100, 200, 100, 200, 100, 200, 100 };
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 2, 2, GL_RG, GL_UNSIGNED_BYTE, data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_GREEN);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		const unsigned char want[4] = { 200, 200, 200, 100 };
		_draw_and_read(prog, t3, "RG8 swizzle(R,R,R,G) [LA8 text]", want);
		glDeleteTextures(1, &t3);
	}

	// 4. R8 with swizzle (R,R,R,R)... used for single-channel glyphs/masks.
	{
		GLuint t4;
		glGenTextures(1, &t4);
		glBindTexture(GL_TEXTURE_2D, t4);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, 2, 2);
		unsigned char data[4] = { 180, 180, 180, 180 };
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 2, 2, GL_RED, GL_UNSIGNED_BYTE, data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_RED);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		const unsigned char want[4] = { 180, 180, 180, 180 };
		_draw_and_read(prog, t4, "R8 swizzle(R,R,R,R)", want);
		glDeleteTextures(1, &t4);
	}

	// 5. Mipmapped min filter on a texture that has its mip chain generated (Godot's default for filtered UI art).
	{
		GLuint t5;
		glGenTextures(1, &t5);
		glBindTexture(GL_TEXTURE_2D, t5);
		unsigned char data[4 * 4] = { 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255 };
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		const unsigned char want[4] = { 0, 0, 255, 255 };
		_draw_and_read(prog, t5, "RGBA8 + generated mipmaps", want);
		glDeleteTextures(1, &t5);
	}

	glDeleteTextures(1, &tex);
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(prog);
	glDeleteShader(vs);
	glDeleteShader(fs);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	fflush(stdout);
}

bool keyboard_show(const char *p_initial, const char *p_guide, int p_max_len, char *r_out, size_t p_out_size) {
	SwkbdConfig kbd;
	if (R_FAILED(swkbdCreate(&kbd, 0))) {
		return false;
	}
	swkbdConfigMakePresetDefault(&kbd);
	if (p_guide && *p_guide) {
		swkbdConfigSetGuideText(&kbd, p_guide);
	}
	if (p_initial && *p_initial) {
		swkbdConfigSetInitialText(&kbd, p_initial);
	}
	swkbdConfigSetStringLenMax(&kbd, (u32)p_max_len);
	r_out[0] = '\0';
	Result rc = swkbdShow(&kbd, r_out, p_out_size);
	swkbdClose(&kbd);
	return R_SUCCEEDED(rc);
}

} // namespace nx
