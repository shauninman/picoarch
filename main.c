#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "core.h"
#include "config.h"
#include "libpicofe/config_file.h"
#include "libpicofe/input.h"
#include "main.h"
#include "menu.h"
#include "plat.h"

#ifdef MMENU
#include <dlfcn.h>
#include <mmenu.h>
#include <SDL/SDL.h>
void* mmenu = NULL;
static int resume_slot = -1;
char save_template_path[MAX_PATH];
#endif

bool should_quit = false;
unsigned current_audio_buffer_size;
char core_name[MAX_PATH];
char* content_path;

static uint32_t vsyncs;
static uint32_t renders;
static float vsyncsps;
static float rendersps;

static void extract_core_name(const char* core_file) {
	char *suffix = NULL;

	strncpy(core_name, basename(core_file), MAX_PATH);
	core_name[sizeof(core_name) - 1] = 0;

	suffix = strrchr(core_name, '_');
	if (suffix && !strcmp(suffix, "_libretro.so"))
		*suffix = 0;
	else {
		suffix = strrchr(core_name, '.');
		if (suffix && !strcmp(suffix, ".so"))
			*suffix = 0;
	}
}

static void toggle_fast_forward(int force_off)
{
	static int frameskip_style_was;
	static int max_frameskip_was;
	static int limit_frames_was;
	static int enable_audio_was;
	static int fast_forward;

	if (force_off && !fast_forward)
		return;

	fast_forward = !fast_forward;

	if (fast_forward) {
		if (!strcmp(core_name, "gpsp")) {
			frameskip_style_was = options_get_value_index("gpsp_frameskip");
			max_frameskip_was = options_get_value_index("gpsp_frameskip_interval");
			options_set_value("gpsp_frameskip", "fixed_interval");
			options_set_value("gpsp_frameskip_interval", "5");
		} else if (!strcmp(core_name, "snes9x2002")) {
			frameskip_style_was = options_get_value_index("snes9x2002_frameskip");
			max_frameskip_was = options_get_value_index("snes9x2002_frameskip_interval");
			options_set_value("snes9x2002_frameskip", "auto");
			options_set_value("snes9x2002_frameskip_interval", "5");
		}

		limit_frames_was = limit_frames;
		enable_audio_was = enable_audio;
		limit_frames = 0;
		enable_audio = 0;
	} else {
		if (!strcmp(core_name, "gpsp")) {
			options_set_value_index("gpsp_frameskip", frameskip_style_was);
			options_set_value_index("gpsp_frameskip_interval", max_frameskip_was);
		} else if (!strcmp(core_name, "snes9x2002")) {
			options_set_value_index("snes9x2002_frameskip", frameskip_style_was);
			options_set_value_index("snes9x2002_frameskip_interval", max_frameskip_was);
		}

		limit_frames = limit_frames_was;
		enable_audio = enable_audio_was;
	}
}

void set_defaults(void)
{
	show_fps = 0;
	limit_frames = 1;
	enable_audio = 1;
	audio_buffer_size = 5;
	scale_size = SCALE_SIZE_NONE;
	scale_filter = SCALE_FILTER_NEAREST;
	scale_update_scaler();

	if (current_audio_buffer_size < audio_buffer_size)
		current_audio_buffer_size = audio_buffer_size;

	for (size_t i = 0; i < core_options.len; i++) {
		const char *key = options_get_key(i);
		if (key)
			core_options.entries[i].value = core_options.entries[i].default_value;
	}

	options_update_changed();
}

int save_config(int is_game)
{
	char config_filename[MAX_PATH];
	FILE *config_file;

	config_file_name(config_filename, MAX_PATH, is_game);
	config_file = fopen(config_filename, "wb");
	if (!config_file) {
		fprintf(stderr, "Could not write config to %s\n", config_filename);
		return -1;
	}

	config_write(config_file);
	config_write_keys(config_file);

	fclose(config_file);
	return 0;
}

static void alloc_config_buffer(char **config_ptr) {
	char config_filename[MAX_PATH];
	FILE *config_file;
	size_t length;

	config_file_name(config_filename, MAX_PATH, 1);
	config_file = fopen(config_filename, "rb");
	if (!config_file) {
		config_file_name(config_filename, MAX_PATH, 0);
		config_file = fopen(config_filename, "rb");
	}

	if (!config_file)
		return;

	PA_INFO("Loading config from %s\n", config_filename);

	fseek(config_file, 0, SEEK_END);
	length = ftell(config_file);
	fseek(config_file, 0, SEEK_SET);

	*config_ptr = calloc(1, length);

	fread(*config_ptr, 1, length, config_file);
	fclose(config_file);
}

void load_config(void)
{
	char *config = NULL;
	alloc_config_buffer(&config);

	if (config) {
		config_read(config);
		free(config);
	}
}

static void load_config_keys(void)
{
	char *config = NULL;
	int kcount = 0;
	const int *defbinds = NULL;

	alloc_config_buffer(&config);

	if (config) {
		config_read_keys(config);
		free(config);

		/* Force input device 0 menu to be bound to the default key */
		in_get_config(0, IN_CFG_BIND_COUNT, &kcount);
		defbinds = in_get_dev_def_binds(0);

		for(int i = 0; i < kcount; i++) {
			if (defbinds[IN_BIND_OFFS(i, IN_BINDTYPE_EMU)] == 1 << EACTION_MENU) {
				in_bind_key(0, i, 1 << EACTION_MENU, IN_BINDTYPE_EMU, 0);
			}
		}
	}
}

void handle_emu_action(emu_action action)
{
	static emu_action prev_action = EACTION_NONE;
	if (prev_action != EACTION_NONE && prev_action == action) return;

	switch (action)
	{
	case EACTION_NONE:
		break;
	case EACTION_MENU:
		toggle_fast_forward(1); /* Force FF off */
		sram_write();
#ifdef MMENU
		if (mmenu) {
			ShowMenu_t ShowMenu = (ShowMenu_t)dlsym(mmenu, "ShowMenu");
			SDL_Surface *screen = SDL_GetVideoSurface();
			MenuReturnStatus status = ShowMenu(content_path, state_allowed() ? save_template_path : NULL, screen, kMenuEventKeyDown);

			if (status==kStatusExitGame) {
				should_quit = 1;
				plat_video_menu_leave();
			}
			else if (status==kStatusOpenMenu) {
				menu_loop();
			}
			else if (status>=kStatusLoadSlot) {
				state_slot = status - kStatusLoadSlot;
				state_read();
			}
			else if (status>=kStatusSaveSlot) {
				state_slot = status - kStatusSaveSlot;
				state_write();
			}

			// release that menu key
			SDL_Event sdlevent;
			sdlevent.type = SDL_KEYUP;
			sdlevent.key.keysym.sym = SDLK_ESCAPE;
			SDL_PushEvent(&sdlevent);
		}
		else {
#endif
			menu_loop();
#ifdef MMENU
		}
#endif
		break;
	case EACTION_TOGGLE_FPS:
		show_fps = !show_fps;
		/* Force the hud to clear */
		plat_video_set_msg(" ");
		break;
	case EACTION_SAVE_STATE:
		state_write();
		break;
	case EACTION_LOAD_STATE:
		state_read();
		break;
	case EACTION_TOGGLE_FF:
		toggle_fast_forward(0);
		break;
	case EACTION_QUIT:
		should_quit = 1;
		break;
	default:
		break;
	}

	prev_action = action;
}

void pa_log(enum retro_log_level level, const char *fmt, ...) {
	char buf[1024] = {0};
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	switch(level) {
#ifdef DEBUG
	case RETRO_LOG_DEBUG:
		printf("DEBUG: %s", buf);
		break;
#endif
	case RETRO_LOG_INFO:
		printf("INFO: %s", buf);
		break;
	case RETRO_LOG_WARN:
		fprintf(stderr, "WARN: %s", buf);
		break;
	case RETRO_LOG_ERROR:
		fprintf(stderr, "ERROR: %s", buf);
		break;
	default:
		break;
	}
}

void pa_track_render(void) {
	renders++;
}

static void count_fps(void)
{
	char msg[HUD_LEN];

	static unsigned int nextsec = 0;
	unsigned int ticks = 0;

	if (show_fps) {
		ticks = plat_get_ticks_ms();
		if (ticks > nextsec) {
			float last_time = (ticks - nextsec + 1000) / 1000.0f;

			if (last_time > 0) {
				vsyncsps = vsyncs / last_time;
				rendersps = renders / last_time;
				vsyncs = 0;
				renders = 0;
				nextsec = ticks + 1000;
			}
		}
		vsyncs++;

		snprintf(msg, HUD_LEN, "FPS: %.1f (%.0f)", rendersps, vsyncsps);
		plat_video_set_msg(msg);
	}
}

static void adjust_audio(void) {
	static unsigned prev_audio_buffer_size = 0;

	if (!prev_audio_buffer_size)
		prev_audio_buffer_size = audio_buffer_size;

	current_audio_buffer_size =
		audio_buffer_size > audio_buffer_size_override
		? audio_buffer_size
		: audio_buffer_size_override;

	if (prev_audio_buffer_size != current_audio_buffer_size) {
		PA_INFO("Resizing audio buffer from %d to %d frames\n",
			prev_audio_buffer_size,
			current_audio_buffer_size);

		plat_sound_resize_buffer();
		prev_audio_buffer_size = current_audio_buffer_size;
	}

	if (current_core.retro_audio_buffer_status) {
		float occupancy = 1.0 - plat_sound_capacity();
		current_core.retro_audio_buffer_status(true, (int)(occupancy * 100), occupancy < 0.50);
	}
}

int main(int argc, char **argv) {
	if (argc != 3) {
		printf("Usage: picoarch LIBRETRO_CORE CONTENT");
		return -1;
	}

	extract_core_name(argv[1]);
	content_path = argv[2];

	set_defaults();

	if (core_load(argv[1])) {
		quit(-1);
	}

	if (core_load_content(argv[2])) {
		quit(-1);
	}

	if (plat_init()) {
		quit(-1);
	}

	/* Must happen after initializing plat_input */
	load_config_keys();

	if (menu_init()) {
		quit(-1);
	}

#ifdef MMENU

	mmenu = dlopen("libmmenu.so", RTLD_LAZY);
	if (mmenu) {
		ResumeSlot_t ResumeSlot = (ResumeSlot_t)dlsym(mmenu, "ResumeSlot");
		if (ResumeSlot) resume_slot = ResumeSlot();
	}

	if (state_allowed() && resume_slot!=-1) {
		state_slot = resume_slot;
		state_read();
		resume_slot = -1;
	}
#endif

	do {
		count_fps();
		adjust_audio();
		current_core.retro_run();
		if (!should_quit)
			plat_video_flip();
	} while (!should_quit);

	return quit(0);
}

int quit(int code) {
	sram_write();
	menu_finish();
	core_unload();
	plat_finish();
	exit(code);
}
