#ifndef __MAIN_H__
#define __MAIN_H__

#include <stdbool.h>
#include "options.h"
#include "libretro.h"

#define MAX_PATH 512

typedef enum {
  EACTION_NONE = 0,
  EACTION_MENU,
  EACTION_SLEEP,
  EACTION_POWER_OFF,
  EACTION_TOGGLE_HUD,
  EACTION_TOGGLE_FF,
  EACTION_SAVE_STATE,
  EACTION_LOAD_STATE,
  EACTION_SCREENSHOT,
  EACTION_QUIT,
} emu_action;

extern bool should_quit;
extern unsigned current_audio_buffer_size;
extern char core_name[MAX_PATH];
extern int config_override;

#ifdef MMENU
extern void* mmenu;
extern char save_template_path[MAX_PATH];
#endif

#ifdef DEBUG_LOGGING
#define PA_DEBUG(...) pa_log(RETRO_LOG_DEBUG, __VA_ARGS__)
#else
#define PA_DEBUG(...)
#endif

#define PA_INFO(...) pa_log(RETRO_LOG_INFO, __VA_ARGS__)
#define PA_WARN(...) pa_log(RETRO_LOG_WARN, __VA_ARGS__)
#define PA_ERROR(...) pa_log(RETRO_LOG_ERROR, __VA_ARGS__)
#define PA_FATAL(...) do { pa_log(RETRO_LOG_ERROR, __VA_ARGS__); quit(-1); } while(0)

int screenshot(void);

void set_defaults(void);
int save_config(int is_game);
void load_config(void);
void load_config_keys(void);
int remove_config(int is_game);

void handle_emu_action(emu_action action);
void pa_log(enum retro_log_level level, const char *fmt, ...);
void pa_track_render(void);
int quit(int code);

#endif /* __MAIN_H__ */
