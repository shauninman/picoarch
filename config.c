#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "main.h"
#include "options.h"
#include "scale.h"

typedef enum {
	CE_TYPE_STR = 0,
	CE_TYPE_NUM = 4,
} config_entry_type;

#define CE_STR(val)	                                                  \
	{ #val, CE_TYPE_STRING, val }

#define CE_NUM(val)	                                                  \
	{ #val, CE_TYPE_NUM, &val }

static const struct {
	const char *name;
	config_entry_type type;
	void *val;
} config_data[] = {
	CE_NUM(show_fps),
	CE_NUM(limit_frames),
	CE_NUM(audio_buffer_size),
	CE_NUM(scale_size),
	CE_NUM(scale_filter),
};

void config_write(FILE *f)
{
	for (size_t i = 0; i < array_size(config_data); i++) {
		switch (config_data[i].type)
		{
		case CE_TYPE_STR:
			fprintf(f, "%s = %s\n", config_data[i].name, (char *)config_data[i].val);
			break;
		case CE_TYPE_NUM:
			fprintf(f, "%s = %u\n", config_data[i].name, *(uint32_t *)config_data[i].val);
			break;
		default:
			PA_WARN("unhandled type %d for %s\n", config_data[i].type, (char *)config_data[i].val);
			break;
		}
	}

	for (size_t i = 0; i < core_options.len; i++) {
		const char* k = options_get_key(i);
		if (!options_is_blocked(k))
			fprintf(f, "%s = %s\n", k, options_get_value(k));
	}
}

static void parse_str_val(char *cval, const char *src)
{
	char *tmp;
	strncpy(cval, src, 256);
	cval[256 - 1] = 0;
	tmp = strchr(cval, '\n');
	if (tmp == NULL)
		tmp = strchr(cval, '\r');
	if (tmp != NULL)
		*tmp = 0;
}

static void parse_num_val(uint32_t *cval, const char *src)
{
	char *tmp = NULL;
	uint32_t val;
	val = strtoul(src, &tmp, 10);
	if (tmp == NULL || src == tmp)
		return; // parse failed

	*cval = val;
}

static char *config_find_value(const char* cfg, const char *key) {
	char *tmp;

	tmp = strstr(cfg, key);
	if (tmp == NULL)
		return NULL;
	tmp += strlen(key);
	if (strncmp(tmp, " = ", 3) != 0)
		return NULL;
	tmp += 3;

	return tmp;
}

void config_read(const char* cfg)
{
	for (size_t i = 0; i < array_size(config_data); i++) {
		char *tmp = config_find_value(cfg, config_data[i].name);
		if (!tmp)
			continue;

		if (config_data[i].type == CE_TYPE_STR) {
			parse_str_val(config_data[i].val, tmp);
			continue;
		}

		parse_num_val(config_data[i].val, tmp);
	}

	for (size_t i = 0; i < core_options.len; i++) {
		char value[256] = {0};
		const char *key = options_get_key(i);
		if (options_is_blocked(key))
			continue;

		char *tmp = config_find_value(cfg, key);
		if (!tmp)
			continue;

		parse_str_val(value, tmp);
		options_set_value(key, value);
	}
}
