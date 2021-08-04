#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "options.h"

int show_fps;
int limit_frames;
int enable_audio;
int audio_buffer_size;
enum scale_size scale_size;
enum scale_filter scale_filter;

struct core_options core_options;

#define MAX_DESC_LEN 20
#define MAX_LINE_LEN 52
#define MAX_LINES 3

static void truncate(char *string, int max_len) {
	size_t len = strlen(string) + 1;
	if (len <= max_len) return;

	strncpy(&string[max_len - 4], "...\0", 4);
}

static void wrap(char *string, size_t max_len, size_t max_lines) {
	char *line = string;

	for (int i = 1; i < max_lines; i++) {
		char *p = line;
		char *prev;
		do {
			prev = p;
			p = strchr(prev+1, ' ');
		} while (p && p - line < max_len);

		if (!p && strlen(line) < max_len) break;

		if (prev && prev != line) {
			line = prev + 1;
			*prev = '\n';
		}
	}
	truncate(line, max_len);

	return;
}

static const char *blocked_options[] = { "gpsp_save_method", NULL};

static bool option_blocked(const char *key) {
	for (int i = 0; blocked_options[i]; i++) {
		if (!strcmp(blocked_options[i], key))
			return true;
	}
	return false;
}

static int options_default_override(const char *key) {
	if (!strcmp(key, "snes9x2002_frameskip")) {
		return 1;
	} else if (!strcmp(key, "snes9x2002_frameskip_interval")) {
		return 3;
	}

	return -1;
}

void options_init(const struct retro_core_option_definition *defs) {
	int i;

	for (i = 0; defs[i].key; i++)
		;

	core_options.visible_len = core_options.len = i;
	core_options.defs = defs;

	core_options.entries = (struct core_option_entry *)calloc(core_options.len, sizeof(struct core_option_entry));
	if (!core_options.entries) {
		PA_ERROR("Error allocating option entries\n");
		options_free();
		return;
	}

	for (i = 0; i < core_options.len; i++) {
		int j, len;
		struct core_option_entry *entry = &core_options.entries[i];

		entry->def = &core_options.defs[i];
		entry->value = options_default_index(entry->def->key);
		entry->prev_value = entry->value;
		entry->blocked = option_blocked(entry->def->key);
		if (entry->blocked)
			core_options.visible_len--;

		len = strlen(entry->def->desc) + 1;
		entry->desc = (char *)calloc(len, sizeof(char));
		if (!entry->desc) {
			PA_ERROR("Error allocating option entries\n");
			options_free();
			return;
		}

		strncpy(entry->desc, entry->def->desc, len);
		truncate(entry->desc, MAX_DESC_LEN);

		if (entry->def->info) {
			len = strlen(entry->def->info) + 1;
			entry->info = (char *)calloc(len, sizeof(char));
			if (!entry->info) {
				PA_ERROR("Error allocating description string\n");
				options_free();
				return;
			}
			strncpy(entry->info, entry->def->info, len);
			wrap(entry->info, MAX_LINE_LEN, MAX_LINES);
		}

		for (j = 0; entry->def->values[j].value; j++)
			;
		j++; /* Make room for NULL entry */

		entry->options = (const char **)calloc(j, sizeof(char *));
		if (!entry->options) {
			PA_ERROR("Error allocating option entries\n");
			options_free();
			return;
		}


		for (j = 0; entry->def->values[j].value; j++) {
			const char *label = entry->def->values[j].label;
			if (!label) {
				label = entry->def->values[j].value;
			}
			entry->options[j] = label;
		}
	}
}

void options_init_variables(const struct retro_variable *vars) {
	int i;

	for (i = 0; vars[i].key; i++)
		;

	core_options.visible_len = core_options.len = i;
	core_options.vars = vars;

	core_options.entries = (struct core_option_entry *)calloc(core_options.len, sizeof(struct core_option_entry));
	if (!core_options.entries) {
		PA_ERROR("Error allocating option entries\n");
		options_free();
		return;
	}

	for (i = 0; i < core_options.len; i++) {
		int j = 0;
		int len;
		struct core_option_entry *entry = &core_options.entries[i];
		char *p;
		char *opt_ptr;
		char *value;

		entry->var = &core_options.vars[i];
		entry->value = options_default_index(entry->var->key);
		entry->prev_value = entry->value;
		entry->blocked = option_blocked(entry->var->key);
		if (entry->blocked)
			core_options.visible_len--;

		len = strlen(entry->var->value) + 1;
		value = (char *)calloc(len, sizeof(char));
		if (!value) {
			PA_ERROR("Error allocating option entries\n");
			options_free();
			return;
		}

		entry->retro_var_value = value;

		strncpy(value, entry->var->value, len);

		p = strchr(value, ';');
		if (p && *(p + 1) == ' ') {
			*p = '\0';
			entry->desc = value;
			truncate(entry->desc, MAX_DESC_LEN);
			p++;
			p++;
		}

		opt_ptr = p;

		for(j = 0; (p = strchr(p, '|')); p++, j++)
			;
		j++; /* Make room for last entry (not ending in |) */
		j++; /* Make room for NULL entry */

		entry->options = (const char **)calloc(j, sizeof(char *));
		if (!entry->options) {
			PA_ERROR("Error allocating option entries\n");
			options_free();
			return;
		}

		p = opt_ptr;
		for (j = 0; (p = strchr(p, '|')); j++) {
			entry->options[j] = opt_ptr;
			*p = '\0';
			p++;
			opt_ptr = p;
		}
		entry->options[j] = opt_ptr;
	}
}

bool options_changed(void) {
	bool was_changed = core_options.changed;
	core_options.changed = false;
	return was_changed;
}

void options_update_changed(void) {
	if (core_options.changed)
		return;

	for(int i = 0; i < core_options.len; i++) {
		struct core_option_entry* entry = &core_options.entries[i];
		if (entry->value != entry->prev_value) {
			core_options.changed = true;
			entry->prev_value = entry->value;
		}
	}
}

const char* options_get_key(int index) {
	if (core_options.defs) {
		return core_options.defs[index].key;
	} else {
		return core_options.vars[index].key;
	}
}

struct core_option_entry* options_get_entry(const char* key) {
	for(int i = 0; i < core_options.len; i++) {
		const char *opt_key = core_options.defs ?
			core_options.defs[i].key :
			core_options.vars[i].key;

		if (!strcmp(opt_key, key)) {
			return &core_options.entries[i];
		}
	}
	return NULL;
}


bool options_is_blocked(const char *key) {
	struct core_option_entry* entry = options_get_entry(key);
	if (entry) {
		return entry->blocked;
	}
	return true;
}

const char* options_get_value(const char* key) {
	struct core_option_entry* entry = options_get_entry(key);
	if (entry) {
		if (entry->def) {
			return entry->def->values[entry->value].value;
		} else {
			return entry->options[entry->value];
		}
	}
	return NULL;
}

int* options_get_value_ptr(const char* key) {
	struct core_option_entry* entry = options_get_entry(key);
	if (entry) {
		return &(entry->value);
	}
	return NULL;
}

int options_get_value_index(const char* key) {
	struct core_option_entry* entry = options_get_entry(key);
	if (entry) {
		return entry->value;
	}
	return 0;
}

void options_set_value(const char* key, const char *value) {
	struct core_option_entry* entry = options_get_entry(key);
	if (entry) {
		const char *option = NULL;
		entry->value = options_default_index(key);

		if (entry->def) {
			for (int i = 0; (option = entry->def->values[i].value); i++) {
				if (!strcmp(option, value)) {
					entry->value = i;
					options_update_changed();
					return;
				}
			}
		} else {
			for (int i = 0; (option = entry->options[i]); i++) {
				if (!strcmp(option, value)) {
					entry->value = i;
					options_update_changed();
					return;
				}
			}
		}
	}
}

void options_set_value_index(const char* key, int value) {
	struct core_option_entry* entry = options_get_entry(key);
	if (entry) {
		entry->value = value;
		options_update_changed();
	}
}

int options_default_index(const char *key) {
	const char *value;
	struct core_option_entry *entry;
	int default_override = options_default_override(key);
	if (default_override >= 0)
		return default_override;

	entry = options_get_entry(key);
	if (!entry || !entry->def)
		return 0;

	for(int i = 0; (value = entry->def->values[i].value); i++) {
		if (!strcmp(value, entry->def->default_value)) {
			return i;
		}
	}
	return 0;
}

const char** options_get_options(const char* key) {
	struct core_option_entry* entry = options_get_entry(key);
	if (entry) {
		return entry->options;
	}
	return NULL;
}

void options_free(void) {
	if (core_options.entries) {
		for (int i = 0; i < core_options.len; i++) {
			struct core_option_entry* entry = &core_options.entries[i];
			if (entry->options) {
				free(entry->options);
			}
			if (entry->retro_var_value) {
				free(entry->retro_var_value);
			} else if (entry->desc) {
				free(entry->desc);
			}
		}
		free(core_options.entries);
	}
	memset(&core_options, 0, sizeof(core_options));
}
