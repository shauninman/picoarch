#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>
#include <string.h>

#define MAX(a, b) (a) > (b) ? (a) : (b)
#define MIN(a, b) (a) < (b) ? (a) : (b)

#define array_size(x) (sizeof(x) / sizeof(x[0]))

static inline bool has_suffix_i(const char *str, const char *suffix) {
	const char *p = strrchr(str, suffix[0]);
	if (!p) p = str;
	return !strcasecmp(p, suffix);
}

void string_truncate(char *string, size_t max_len);
void string_wrap(char *string, size_t max_len, size_t max_lines);

#endif
