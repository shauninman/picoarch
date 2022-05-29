#ifndef CONTENT_H
#define CONTENT_H

#include "libretro.h"
#include "main.h"

struct content {
	const char path[MAX_PATH];
	bool hasm3u; 
	char *tmpfile;
	void *data;
	size_t size;
};

struct content *content_init(const char *path);
void content_based_name(const struct content *content,
                        char *buf, size_t len,
                        const char *basedir, const char *subdir,
                        const char *new_extension);
int content_load_game_info(struct content *content, struct retro_game_info *info, bool needs_fullpath);
void content_free(struct content *content);

#endif
