#ifndef _CORE_H__
#define _CORE_H__

#include "libretro.h"

struct core_cbs {
	bool initialized;
	void *handle;
	void (*retro_init)(void);
	void (*retro_deinit)(void);
	void (*retro_api_version)(void);
	void (*retro_get_system_info)(struct retro_system_info *info);
	void (*retro_get_system_av_info)(struct retro_system_av_info *info);
	void (*retro_set_controller_port_device)(unsigned port, unsigned device);
	void (*retro_reset)(void);
	void (*retro_run)(void);
	size_t (*retro_serialize_size)(void);
	bool (*retro_serialize)(void *data, size_t size);
	bool (*retro_unserialize)(const void *data, size_t size);
	void (*retro_cheat_reset)(void);
	void (*retro_cheat_set)(unsigned index, bool enabled, const char *code);
	bool (*retro_load_game)(const struct retro_game_info *game);
	bool (*retro_load_game_special)(unsigned game_type, const struct retro_game_info *info, size_t num_info);
	void (*retro_unload_game)(void);
	unsigned (*retro_get_region)(void);
	void *(*retro_get_memory_data)(unsigned id);
	size_t (*retro_get_memory_size)(unsigned id);
	retro_audio_buffer_status_callback_t retro_audio_buffer_status;
};

extern struct core_cbs current_core;

extern double sample_rate;
extern double frame_rate;
extern double aspect_ratio;
extern unsigned audio_buffer_size_override;
extern int state_slot;

void config_file_name(char *buf, size_t len, int is_game);
void save_relative_path(char *buf, size_t len, const char *basename);

void sram_read(void);
void sram_write(void);

bool state_allowed(void);
void state_file_name(char *name, size_t size, int slot);
int state_read(void);
int state_write(void);

int core_load(const char *corefile);
int core_load_content(const char *path);
void core_unload(void);

#endif
