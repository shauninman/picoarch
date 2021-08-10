#ifndef _OPTIONS_H__
#define _OPTIONS_H__
#include "libretro.h"
#include "scale.h"

extern int show_fps;
extern int limit_frames;
extern int enable_audio;
extern unsigned audio_buffer_size;
extern enum scale_size scale_size;
extern enum scale_filter scale_filter;

struct core_option_entry {
	char *key;
	int value;
	int prev_value;
	char *desc;
	char *retro_var_value;
	bool blocked;
	char *info;
	const char **options;
	const struct retro_core_option_definition *def;
};

struct core_options {
	size_t len;
	size_t visible_len;
	bool changed;
	struct core_option_entry *entries;
	const struct retro_core_option_definition *defs;
};

extern struct core_options core_options;

void options_init(const struct retro_core_option_definition *defs);
void options_init_variables(const struct retro_variable *vars);
bool options_changed(void);
void options_update_changed(void);
const char* options_get_key(int index);

struct core_option_entry* options_get_entry(const char* key);
bool options_is_blocked(const char *key);

const char* options_get_value(const char* key);
int* options_get_value_ptr(const char* key);
int options_get_value_index(const char* key);

void options_set_value(const char* key, const char *value);
void options_set_value_index(const char* key, int value);

int options_default_index(const char *key);
const char** options_get_options(const char* key);
void options_free(void);

#endif
