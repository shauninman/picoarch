// #include <png.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "core.h"
#include "config.h"
#include "content.h"
#include "libpicofe/config_file.h"
#include "libpicofe/input.h"
#include "main.h"
#include "menu.h"
#include "overrides.h"
#include "plat.h"
#include "util.h"

#ifdef MMENU
#include <dlfcn.h>
#include <mmenu.h>
#include <SDL/SDL.h>
void* mmenu = NULL;
char save_template_path[MAX_PATH];
#endif

bool should_quit = false;
unsigned current_audio_buffer_size;
char core_name[MAX_PATH];
int config_override = 0;
static int last_screenshot = 0;

static uint32_t vsyncs;
static uint32_t renders;

#define UNDERRUN_THRESHOLD 50

static void toggle_fast_forward(int force_off)
{
	static int frameskip_style_was;
	static int max_frameskip_was;
	static int limit_frames_was;
	static int enable_audio_was;
	static int fast_forward;
	const struct core_override *override = get_overrides();

	if (force_off && !fast_forward)
		return;

	fast_forward = !fast_forward;

	if (fast_forward) {
		if (override && override->fast_forward) {
			const char *type_key = override->fast_forward->type_key;
			const char *interval_key = override->fast_forward->interval_key;

			frameskip_style_was = options_get_value_index(type_key);
			max_frameskip_was = options_get_value_index(interval_key);
			options_set_value(
				type_key,
				CORE_OVERRIDE(override->fast_forward, type_value, "fixed_interval"));
			options_set_value(
				interval_key,
				CORE_OVERRIDE(override->fast_forward, interval_value, "5"));
		}

		limit_frames_was = limit_frames;
		enable_audio_was = enable_audio;
		limit_frames = 0;
		enable_audio = 0;
	} else {
		if (override && override->fast_forward) {
			const char *type_key = override->fast_forward->type_key;
			const char *interval_key = override->fast_forward->interval_key;

			options_set_value_index(type_key, frameskip_style_was);
			options_set_value_index(interval_key, max_frameskip_was);
		}

		limit_frames = limit_frames_was;
		enable_audio = enable_audio_was;
	}
}

// static int screenshot_file_name(char *name, size_t len) {
// 	char suffix[MAX_PATH];
//
// 	for (int i = last_screenshot; i < 10000; i++) {
// 		snprintf(suffix, MAX_PATH, "IMG_%04d.png", i);
// 		save_relative_path(name, len, suffix);
//
// 		if (access(name, F_OK) == -1) {
// 			last_screenshot = i;
// 			return 0;
// 		}
// 	}
// 	*name = '\0';
// 	return -1;
// }
//
// static int png_write_rgb565(const uint16_t *buf, png_structp png_ptr, int w, int h) {
// 	png_byte *row_pointer = calloc(w * 3, sizeof(png_byte));
// 	int ret = -1;
//
// 	if (!row_pointer)
// 		return ret;
//
// 	if (setjmp(png_jmpbuf(png_ptr)))
// 		goto finish;
//
// 	for (int i = 0; i < h; i++) {
// 		uint16_t *pbuf = &((uint16_t *)buf)[i * w];
// 		png_byte *prow = row_pointer;
//
// 		for (int j = 0; j < w; j++) {
// 			uint16_t px = *pbuf++;
// 			*prow++ = ((((px & 0xF800) >> 11) * 255 + 15) / 31);
// 			*prow++ = ((((px & 0x07E0) >> 5)  * 255 + 31) / 63);
// 			*prow++ = ((((px & 0x001F))       * 255 + 15) / 31);
// 		}
// 		png_write_row(png_ptr, row_pointer);
// 	}
// 	ret = 0;
//
// finish:
// 	if (row_pointer)
// 		free(row_pointer);
//
// 	return ret;
// }
//
// static int write_png(const uint16_t *buf, int w, int h, FILE *file) {
// 	png_structp png_ptr = NULL;
// 	png_infop info_ptr = NULL;
// 	int ret = -1;
//
// 	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
// 	if (!png_ptr)
// 		goto finish;
//
// 	info_ptr = png_create_info_struct(png_ptr);
// 	if (!info_ptr)
// 		goto finish;
//
// 	if (setjmp(png_jmpbuf(png_ptr)))
// 		goto finish;
//
// 	png_init_io(png_ptr, file);
//
// 	png_set_IHDR(png_ptr, info_ptr, w, h, 8,
// 	             PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
// 	             PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
//
// 	png_write_info(png_ptr, info_ptr);
//
// 	if (png_write_rgb565(buf, png_ptr, w, h))
// 		goto finish;
//
// 	if (setjmp(png_jmpbuf(png_ptr)))
// 		goto finish;
//
// 	png_write_end(png_ptr, info_ptr);
// 	ret = 0;
//
// finish:
// 	png_destroy_write_struct(&png_ptr, &info_ptr);
// 	return ret;
// }

int screenshot(void) {
	return 0;
// 	FILE *fp;
// 	char filename[MAX_PATH];
// 	int w, h;
// 	void *buf = plat_prepare_screenshot(&w, &h, NULL);
// 	int ret = -1;
//
// 	if (screenshot_file_name(filename, MAX_PATH)) {
// 		PA_ERROR("No available filename for screenshot\n");
// 		return -1;
// 	}
//
// 	fp = fopen(filename, "wb");
// 	if (!fp)
// 		goto finish;
//
// 	if (write_png(buf, w, h, fp))
// 		goto finish;
//
// 	PA_INFO("Wrote screenshot to %s\n", filename);
// 	ret = 0;
//
// finish:
// 	if (fp)
// 		fclose(fp);
// 	return ret;
}

void set_defaults(void)
{
	show_fps = 0;
	show_cpu = 0;
	show_hud = 1;
	limit_frames = 1;
	enable_audio = 1;
	enable_drc = 0;
	audio_buffer_size = 5;
	scale_size = SCALE_SIZE_NONE;
	// scale_filter = SCALE_FILTER_NEAREST;
	scale_effect = default_scale_effect;
	optimize_text = 1;
	// max_upscale = 8;
	
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

	if (is_game)
		config_override = 1;

	return 0;
}

static void alloc_config_buffer(char **config_ptr) {
	char config_filename[MAX_PATH];
	FILE *config_file;
	size_t length;
	config_override = 0;

	config_file_name(config_filename, MAX_PATH, 1);
	config_file = fopen(config_filename, "rb");
	if (config_file) {
		config_override = 1;
	} else {
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
	plat_reinit();
}

void load_config_keys(void)
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

int remove_config(int is_game) {
	char config_filename[MAX_PATH];
	int ret;

	config_file_name(config_filename, MAX_PATH, is_game);
	ret = remove(config_filename);
	if (ret == 0)
		config_override = 0;

	return ret;
}

static void autosave(void) {
	// autosave
	int last_state_slot = state_slot;
	state_slot = 9; // only supports up to 10 states :cold_sweat: (good thing the UI only supports 8 :lolsob:)
	state_write();
	state_slot = last_state_slot;
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
	case EACTION_SLEEP:
	case EACTION_POWER_OFF:
		toggle_fast_forward(1); /* Force FF off */
		sram_write();
#ifdef MMENU
		if (mmenu && content && content->path) {
			ShowMenu_t ShowMenu = (ShowMenu_t)dlsym(mmenu, "ShowMenu");
			MenuRequestState requested_state = action==EACTION_SLEEP ? kRequestSleep : (action==EACTION_POWER_OFF ? kRequestPowerOff : kRequestMenu);
			MenuReturnStatus status = ShowMenu(content->path, state_allowed() ? save_template_path : NULL, plat_clean_screen(), requested_state, autosave);
			char disc_path[256];
			ChangeDisc_t ChangeDisc = (ChangeDisc_t)dlsym(mmenu, "ChangeDisc");
			
			if (status == kStatusPowerOff) {
				should_quit = 1;
			}
			else if (status == kStatusExitGame) {
				should_quit = 1;
				plat_video_menu_leave();
			} else if (status == kStatusResetGame) {
				current_core.retro_reset();
			} else if (status == kStatusChangeDisc && ChangeDisc(disc_path)) {
				disc_replace_index(&content, 0, disc_path);
			} else if (status == kStatusOpenMenu) {
				plat_video_flip();
				menu_loop();
			} else if (status >= kStatusLoadSlot) {
				if (ChangeDisc(disc_path)) disc_replace_index(&content, 0, disc_path);
				state_slot = status - kStatusLoadSlot;
				state_read();
			} else if (status >= kStatusSaveSlot) {
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
	case EACTION_TOGGLE_HUD:
		show_hud = !show_hud;
		/* Force the hud to clear */
		plat_video_set_msg(NULL, 0, 0);
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
	case EACTION_SCREENSHOT:
		screenshot();
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

static void show_startup_message(void) {
	const struct core_override *override = get_overrides();
	if (override && override->startup_msg) {
		plat_video_set_msg(override->startup_msg->msg, 2, override->startup_msg->msec);
	}
}

void pa_track_render(void) {
	renders++;
}

#define CPU_MSG_LEN 10
static void count_fps(void)
{
	char msg[HUD_LEN];
	static unsigned int nextsec = 0;
	static unsigned last_cpu_ticks = 0;
	unsigned int ticks = 0;

	if (show_hud && (show_fps || show_cpu)) {
		ticks = plat_get_ticks_ms();
		if (ticks > nextsec) {
			float last_time = (ticks - nextsec + 1000) / 1000.0f;

			if (last_time > 0) {
				char cpu_msg[CPU_MSG_LEN];
				char fps_msg[HUD_LEN - CPU_MSG_LEN];

				cpu_msg[0] = fps_msg[0] = '\0';
				nextsec = ticks + 1000;

				if (show_fps) {
					float vsyncsps = vsyncs / last_time;
					float rendersps = renders / last_time;
					vsyncs = 0;
					renders = 0;
					snprintf(fps_msg, sizeof(fps_msg), "FPS: %.1f (%.0f)", rendersps, vsyncsps);
				}

				if (show_cpu) {
					unsigned cpu_ticks = plat_cpu_ticks();
					if (cpu_ticks && last_cpu_ticks) {
						float cpu_percent = (cpu_ticks - last_cpu_ticks) / last_time;
						snprintf(cpu_msg, sizeof(cpu_msg), "%.1f%%", cpu_percent);
					}
					last_cpu_ticks = cpu_ticks;
				}

				snprintf(msg, HUD_LEN, "%-*s%*s",
				         (HUD_LEN - CPU_MSG_LEN - 1), fps_msg,
				         CPU_MSG_LEN - 1, cpu_msg);
				plat_video_set_msg(msg, 1, 1100);
			}
		}
		vsyncs++;

	}
}

static void adjust_audio(void) {
	static unsigned prev_audio_buffer_size = 0;

	if (!prev_audio_buffer_size)
		prev_audio_buffer_size = current_audio_buffer_size;

	current_audio_buffer_size = MAX(audio_buffer_size, audio_buffer_size_override);

	if (prev_audio_buffer_size != current_audio_buffer_size) {
		PA_INFO("Resizing audio buffer from %d to %d frames\n",
			prev_audio_buffer_size,
			current_audio_buffer_size);

		plat_sound_resize_buffer();
		prev_audio_buffer_size = current_audio_buffer_size;
	}

	if (current_core.retro_audio_buffer_status) {
		int occupancy = plat_sound_occupancy();
		if (enable_drc)
			occupancy = MIN(100, occupancy * 2);

		current_core.retro_audio_buffer_status(true, occupancy, occupancy < UNDERRUN_THRESHOLD);
	}
}

static void get_tag_name(const char* in_path, char* out_tag) {
	char* tmp;
	strcpy(out_tag, in_path);
	tmp = out_tag;
	char roms_path[MAX_PATH];
	sprintf(roms_path, "%s/Roms/", getenv("SDCARD_PATH"));
	
	// extract just the Roms folder name
	tmp += strlen(roms_path) + 1;
	char* tmp2 = strchr(tmp, '/');
	if (tmp2) tmp2[0] = '\0';

	// finally extract pak name from parenths if present
	tmp = strrchr(tmp, '(');
	if (tmp) {
		tmp += 1;
		strcpy(out_tag, tmp);
		tmp = strchr(out_tag,')');
		tmp[0] = '\0';
	}
}

int main(int argc, char **argv) {
	char content_path[MAX_PATH];
	char tag_name[MAX_PATH];

	if (argc > 1) {
		if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
			printf("Usage: picoarch libretro_core content [NONE DMG LCD SCANLINE]\n");
			return 0;
		}
	}

	if (plat_init()) {
		quit(-1);
	}

	if (menu_init()) {
		quit(-1);
	}

	if (argc > 1 && argv[1]) {
		strncpy(core_path, argv[1], sizeof(core_path) - 1);
	} else {
		quit(-1);
	}
	
	if (argc > 2 && argv[2]) {
		strncpy(content_path, argv[2], sizeof(content_path) - 1);
	} else {
		quit(-1);
	}
	
	if (argc > 3 && argv[3]) {
		if (!strcmp(argv[3],"NONE")) default_scale_effect = SCALE_EFFECT_NONE;
		else if (!strcmp(argv[3],"DMG")) default_scale_effect = SCALE_EFFECT_DMG;
		else if (!strcmp(argv[3],"LCD")) default_scale_effect = SCALE_EFFECT_LCD;
		else default_scale_effect = SCALE_EFFECT_SCANLINE;
	}
	
	get_tag_name(content_path, tag_name);

	core_extract_name(core_path, core_name, sizeof(core_name));

	if (core_open(core_path, tag_name)) {
		quit(-1);
	}

	content = content_init(content_path);
	if (!content) {
		PA_ERROR("Couldn't allocate memory for content path\n");
		quit(-1);
	}

	set_defaults();
	load_config();
	core_load();

	if (core_load_content(content)) {
		quit(-1);
	}

	load_config_keys();

#ifdef MMENU
	mmenu = dlopen("libmmenu.so", RTLD_LAZY);
	if (mmenu) {
		if (state_allowed()) {
			ResumeSlot_t ResumeSlot = (ResumeSlot_t)dlsym(mmenu, "ResumeSlot");
			if (ResumeSlot) resume_slot = ResumeSlot();
		}
		else {
			ShowWarning_t ShowWarning = (ShowWarning_t)dlsym(mmenu, "ShowWarning");
			if (ShowWarning) ShowWarning();
		}
	}
#endif
	show_startup_message();
	state_resume();

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
	menu_finish();
	core_unload();
	plat_finish();
	exit(code);
}
