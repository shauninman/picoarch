#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cheat.h"
#include "main.h"
#include "util.h"

#define MAX_DESC_LEN 27
#define MAX_LINE_LEN 52
#define MAX_LINES 3

static size_t parse_count(FILE *file) {
	size_t count = 0;
	fscanf(file, " cheats = %ld\n", &count);
	return count;
}

static const char *find_val(const char *start) {
	start--;
	while(!isspace(*++start))
		;

	while(isspace(*++start))
		;

	if (*start != '=')
		return NULL;

	while(isspace(*++start))
		;

	return start;
}

static int parse_bool(const char *ptr, int *out) {
	if (!strncasecmp(ptr, "true", 4)) {
		*out = 1;
	} else if (!strncasecmp(ptr, "false", 5)) {
		*out = 0;
	} else {
		return -1;
	}

	return 0;
}

static int parse_string(const char *ptr, char *buf, size_t len) {
	int index = 0;
	size_t input_len = strlen(ptr);

	buf[0] = '\0';

	if (*ptr++ != '"')
		return -1;

	while (*ptr != '\0' && *ptr != '"' && index < len - 1) {
		if (*ptr == '\\' && index < input_len - 1) {
			ptr++;
			buf[index++] = *ptr++;
		} else if (*ptr == '&' && !strncmp(ptr, "&quot;", 6)) {
			buf[index++] = '"';
			ptr += 6;
		} else {
			buf[index++] = *ptr++;
		}
	}

	if (*ptr != '"') {
		buf[0] = '\0';
		return -1;
	}

	buf[index] = '\0';
	return 0;
}

static int parse_cheats(struct cheats *cheats, FILE *file) {
	int ret = -1;
	char line[512];
	char buf[512];
	const char *ptr;

	do {
		if (!fgets(line, sizeof(line), file)) {
			ret = 0;
			break;
		}

		if (line[strlen(line) - 1] != '\n' && !feof(file)) {
			PA_WARN("Cheat line too long\n");
			continue;
		}

		if ((ptr = strstr(line, "cheat"))) {
			int index = -1;
			struct cheat *cheat;
			size_t len;
			sscanf(ptr, "cheat%d", &index);

			if (index >= cheats->count)
				continue;
			cheat = &cheats->cheats[index];

			if (strstr(ptr, "_desc")) {
				ptr = find_val(ptr);
				if (!ptr || parse_string(ptr, buf, sizeof(buf))) {
					PA_WARN("Couldn't parse cheat %d description\n", index);
					continue;
				}

				len = strlen(buf);
				if (len == 0)
					continue;

				cheat->name = calloc(len+1, sizeof(char));
				if (!cheat->name)
					goto finish;

				strncpy((char *)cheat->name, buf, len);
				string_truncate((char *)cheat->name, MAX_DESC_LEN);

				if (len >= MAX_DESC_LEN) {
					cheat->info = calloc(len+1, sizeof(char));
					if (!cheat->info)
						goto finish;

					strncpy((char *)cheat->info, buf, len);
					string_wrap((char *)cheat->info, MAX_LINE_LEN, MAX_LINES);
				}
			} else if (strstr(ptr, "_code")) {
				ptr = find_val(ptr);
				if (!ptr || parse_string(ptr, buf, sizeof(buf))) {
					PA_WARN("Couldn't parse cheat %d code\n", index);
					continue;
				}

				len = strlen(buf);
				if (len == 0)
					continue;

				cheat->code = calloc(len+1, sizeof(char));
				if (!cheat->code)
					goto finish;

				strncpy((char *)cheat->code, buf, len);
			} else if (strstr(ptr, "_enable")) {
				ptr = find_val(ptr);
				if (!ptr || parse_bool(ptr, &cheat->enabled)) {
					PA_WARN("Couldn't parse cheat %d enabled\n", index);
					continue;
				}
			}
		}
	} while(1);

finish:
	return ret;
}

struct cheats *cheats_load(const char *filename) {
	int success = 0;
	struct cheats *cheats = NULL;
	FILE *file = NULL;
	size_t i;

	file = fopen(filename, "r");
	if (!file)
		goto finish;

	PA_INFO("Loading cheats from %s\n", filename);

	cheats = calloc(1, sizeof(struct cheats));
	if (!cheats) {
		PA_ERROR("Couldn't allocate memory for cheats\n");
		goto finish;
	}

	cheats->count = parse_count(file);
	if (cheats->count <= 0) {
		PA_ERROR("Couldn't read cheat count\n");
		goto finish;
	}

	cheats->cheats = calloc(cheats->count, sizeof(struct cheat));
	if (!cheats->cheats) {
		PA_ERROR("Couldn't allocate memory for cheats\n");
		goto finish;
	}

	if (parse_cheats(cheats, file)) {
		PA_ERROR("Error reading cheat %d\n", i);
		goto finish;
	}

	success = 1;
finish:

	if (!success) {
		cheats_free(cheats);
		cheats = NULL;
	}

	if (file)
		fclose(file);

	return cheats;
}

void cheats_free(struct cheats *cheats) {
	size_t i;
	if (cheats) {
		for (i = 0; i < cheats->count; i++) {
			struct cheat *cheat = &cheats->cheats[i];
			if (cheat) {
				free((char *)cheat->name);
				free((char *)cheat->info);
				free((char *)cheat->code);
			}
		}
		free(cheats->cheats);
	}
	free(cheats);
}
