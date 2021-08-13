#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "options.h"

int show_fps;
int limit_frames;
int enable_audio;
unsigned audio_buffer_size;
enum scale_size scale_size;
enum scale_filter scale_filter;

struct core_options core_options;

#define MAX_DESC_LEN 20
#define MAX_LINE_LEN 52
#define MAX_LINES 3

static void truncate(char *string, size_t max_len) {
	size_t len = strlen(string) + 1;
	if (len <= max_len) return;

	strncpy(&string[max_len - 4], "...\0", 4);
}

static void wrap(char *string, size_t max_len, size_t max_lines) {
	char *line = string;

	for (size_t i = 1; i < max_lines; i++) {
		char *p = line;
		char *prev;
		do {
			prev = p;
			p = strchr(prev+1, ' ');
		} while (p && p - line < (int)max_len);

		if (!p && strlen(line) < max_len) break;

		if (prev && prev != line) {
			line = prev + 1;
			*prev = '\n';
		}
	}
	truncate(line, max_len);

	return;
}

static const char *blocked_options[] = {
	"gambatte_rumble_level",
	"gambatte_show_gb_link_settings",
	"gpsp_save_method",
	NULL
};

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
	} else if (!strcmp(key, "mame2000-frameskip")) {
		return 1;
	} else if (!strcmp(key, "mame2000-frameskip_interval")) {
		return 1;
	} else if (!strcmp(key, "mame2000-skip_disclaimer")) {
		return 1;
	} else if (!strcmp(key, "mame2000-sample_rate")) {
		return 1;
	}

	return -1;
}


static int options_default_index(const char *key, const struct retro_core_option_definition *def) {
	const char *value;
	int default_override = options_default_override(key);
	if (default_override >= 0)
		return default_override;

	if (!def)
		return 0;

	for(int i = 0; (value = def->values[i].value); i++) {
		if (!strcmp(value, def->default_value)) {
			return i;
		}
	}
	return 0;
}

void options_init(const struct retro_core_option_definition *defs) {
	size_t i;

	for (i = 0; defs[i].key; i++)
		;

	core_options.visible_len = core_options.len = i;

	core_options.entries = (struct core_option_entry *)calloc(core_options.len, sizeof(struct core_option_entry));
	if (!core_options.entries) {
		PA_ERROR("Error allocating option entries\n");
		options_free();
		return;
	}

	for (i = 0; i < core_options.len; i++) {
		int j, len;
		const struct retro_core_option_definition *def = &defs[i];
		struct core_option_entry *entry = &core_options.entries[i];

		len = strlen(def->key) + 1;
		entry->key = (char *)calloc(len, sizeof(char));
		if (!entry->key) {
			PA_ERROR("Error allocating option entries\n");
			options_free();
			return;
		}
		strncpy(entry->key, def->key, len);

		entry->value = options_default_index(def->key, def);
		entry->prev_value = entry->value;
		entry->default_value = entry->value;
		entry->blocked = option_blocked(def->key);
		if (entry->blocked)
			core_options.visible_len--;
		entry->visible = true;

		len = strlen(def->desc) + 1;
		entry->desc = (char *)calloc(len, sizeof(char));
		if (!entry->desc) {
			PA_ERROR("Error allocating option entries\n");
			options_free();
			return;
		}

		strncpy(entry->desc, def->desc, len);
		truncate(entry->desc, MAX_DESC_LEN);

		if (def->info) {
			len = strlen(def->info) + 1;
			entry->info = (char *)calloc(len, sizeof(char));
			if (!entry->info) {
				PA_ERROR("Error allocating description string\n");
				options_free();
				return;
			}
			strncpy(entry->info, def->info, len);
			wrap(entry->info, MAX_LINE_LEN, MAX_LINES);
		}

		for (j = 0; def->values[j].value; j++)
			;
		j++; /* Make room for NULL entry */

		entry->values = (char **)calloc(j, sizeof(char *));
		if (!entry->values) {
			PA_ERROR("Error allocating option entries\n");
			options_free();
			return;
		}

		entry->labels = (char **)calloc(j, sizeof(char *));
		if (!entry->labels) {
			PA_ERROR("Error allocating option entries\n");
			options_free();
			return;
		}

		for (j = 0; def->values[j].value; j++) {
			const char *value = def->values[j].value;
			size_t value_len = strlen(value);
			const char *label = def->values[j].label;
			size_t label_len = 0;

			entry->values[j] = (char *)calloc(value_len + 1, sizeof(char));
			if (!entry->values[j]) {
				PA_ERROR("Error allocating memory for option values");
				options_free();
				return;
			}

			strncpy(entry->values[j], value, value_len);

			if (label) {
				label_len = strlen(label);

				entry->labels[j] = (char *)calloc(label_len + 1, sizeof(char));
				if (!entry->labels[j]) {
					PA_ERROR("Error allocating memory for option labels");
					options_free();
					return;
				}

				strncpy(entry->labels[j], label, label_len);
			} else {
				entry->labels[j] = entry->values[j];
			}
		}
	}
}

void options_init_variables(const struct retro_variable *vars) {
	size_t i;

	for (i = 0; vars[i].key; i++)
		;

	core_options.visible_len = core_options.len = i;

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
		const struct retro_variable *var = &vars[i];
		char *p;
		char *opt_ptr;
		char *value;

		len = strlen(var->key) + 1;
		entry->key = (char *)calloc(len, sizeof(char));
		if (!entry->key) {
			PA_ERROR("Error allocating option entries\n");
			options_free();
			return;
		}
		strncpy(entry->key, var->key, len);

		entry->value = options_default_index(var->key, NULL);
		entry->prev_value = entry->value;
		entry->default_value = entry->value;
		entry->blocked = option_blocked(var->key);
		if (entry->blocked)
			core_options.visible_len--;

		entry->visible = true;
		len = strlen(var->value) + 1;
		value = (char *)calloc(len, sizeof(char));
		if (!value) {
			PA_ERROR("Error allocating option entries\n");
			options_free();
			return;
		}

		entry->retro_var_value = value;

		strncpy(value, var->value, len);

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

		entry->values = (char **)calloc(j, sizeof(char *));
		if (!entry->values) {
			PA_ERROR("Error allocating option entries\n");
			options_free();
			return;
		}

		entry->labels = (char **)calloc(j, sizeof(char *));
		if (!entry->labels) {
			PA_ERROR("Error allocating option entries\n");
			options_free();
			return;
		}

		p = opt_ptr;
		for (j = 0; (p = strchr(p, '|')); j++) {
			entry->values[j] = opt_ptr;
			entry->labels[j] = opt_ptr;
			*p = '\0';
			p++;
			opt_ptr = p;
		}
		entry->values[j] = opt_ptr;
		entry->labels[j] = opt_ptr;
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

	for(size_t i = 0; i < core_options.len; i++) {
		struct core_option_entry* entry = &core_options.entries[i];
		if (entry->value != entry->prev_value) {
			core_options.changed = true;
			entry->prev_value = entry->value;
		}
	}
}

const char* options_get_key(int index) {
	return core_options.entries[index].key;
}

struct core_option_entry* options_get_entry(const char* key) {
	for(size_t i = 0; i < core_options.len; i++) {
		const char *opt_key = options_get_key(i);

		if (!strcmp(opt_key, key)) {
			return &core_options.entries[i];
		}
	}
	return NULL;
}

const char* options_get_value(const char* key) {
	struct core_option_entry* entry = options_get_entry(key);
	if (entry)
		return entry->values[entry->value];

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
		char *option;
		entry->value = entry->default_value;

		for (int i = 0; (option = entry->values[i]); i++) {
			if (!strcmp(option, value)) {
				entry->value = i;
				options_update_changed();
				return;
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

void options_set_visible(const char* key, bool visible) {
	struct core_option_entry* entry = options_get_entry(key);
	if (entry) {
		entry->visible = visible;
	}
}

const char** options_get_options(const char* key) {
	struct core_option_entry* entry = options_get_entry(key);
	if (entry) {
		return (const char **)entry->labels;
	}
	return NULL;
}

void options_free(void) {
	if (core_options.entries) {
		for (size_t i = 0; i < core_options.len; i++) {
			struct core_option_entry* entry = &core_options.entries[i];
			if (entry->retro_var_value) {
				/* option values / labels are all pointers into retro_var_value,
				 * no need to free them one by one */
				free(entry->retro_var_value);
			} else {
				if (entry->labels) {
					char *label;
					for (int j = 0; (label = entry->labels[j]); j++) {
						if (label != entry->values[j])
							free(label);
					}
				}

				if (entry->values) {
					char *value;
					for (int j = 0; (value = entry->values[j]); j++) {
					        free(value);
					}
				}

				if (entry->desc)
					free(entry->desc);
			}

			if (entry->labels)
				free(entry->labels);

			if (entry->values)
				free(entry->values);

			if (entry->key)
				free(entry->key);
		}
		free(core_options.entries);
	}
	memset(&core_options, 0, sizeof(core_options));
}
