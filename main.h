#ifndef __MAIN_H__
#define __MAIN_H__

#include <stdbool.h>
#include <string.h>
#include "options.h"
#include "libretro.h"

#define MAX_PATH 512

typedef enum {
  EACTION_NONE = 0,
  EACTION_MENU,
  EACTION_TOGGLE_FPS,
  EACTION_TOGGLE_FF,
  EACTION_SAVE_STATE,
  EACTION_LOAD_STATE,
  EACTION_QUIT,
} emu_action;

extern bool should_quit;
extern int current_audio_buffer_size;
extern char core_name[MAX_PATH];
extern char* content_path;

#ifdef MMENU
extern void* mmenu;
extern char save_template_path[MAX_PATH];
#endif


#define MAX(a, b) (a) > (b) ? (a) : (b)
#define MIN(a, b) (a) < (b) ? (a) : (b)

static inline bool has_suffix_i(const char *str, const char *suffix) {
	const char *p = strrchr(str, suffix[0]);
	if (!p) p = str;
	return !strcasecmp(p, suffix);
}

#define array_size(x) (sizeof(x) / sizeof(x[0]))

#ifdef DEBUG_LOGGING
#define PA_DEBUG(...) pa_log(RETRO_LOG_DEBUG, __VA_ARGS__)
#else
#define PA_DEBUG(...)
#endif

#define PA_INFO(...) pa_log(RETRO_LOG_INFO, __VA_ARGS__)
#define PA_WARN(...) pa_log(RETRO_LOG_WARN, __VA_ARGS__)
#define PA_ERROR(...) pa_log(RETRO_LOG_ERROR, __VA_ARGS__)
#define PA_FATAL(...) do { pa_log(RETRO_LOG_ERROR, __VA_ARGS__); quit(-1); } while(0)


void set_defaults(void);
int save_config(int is_game);
void load_config(void);

void handle_emu_action(emu_action action);
void pa_log(enum retro_log_level level, const char *fmt, ...);
void pa_track_render(void);
int quit(int code);

#endif /* __MAIN_H__ */
