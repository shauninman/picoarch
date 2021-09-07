#ifndef _OVERRIDES_H__
#define _OVERRIDES_H__

#include "libpicofe/menu.h"
#include "libretro.h"

struct core_override_option {
	const char *key;
	const char *desc;
	const char *info;
	const bool blocked;
	const char *default_value;
	const char *retro_var_value;
	struct retro_core_option_value options[RETRO_NUM_CORE_OPTION_VALUES_MAX];
};

struct core_override_fast_forward {
	const char *type_key;
	const char *type_value;
	const char *interval_key;
	const char *interval_value;
};

struct core_override_startup_msg {
	const char *msg;
	const unsigned msec;
};

struct core_override {
	const char *core_name;
	const struct core_override_fast_forward *fast_forward;
	const struct core_override_startup_msg *startup_msg;
	me_bind_action* actions;
	const size_t action_size;
	const struct core_override_option* options;
	int block_load_content;
};

#define CORE_OVERRIDE(override, key, fallback) ((override && override->key) ? (override->key) : (fallback))

const struct core_override *get_overrides(void);
void set_overrides(const char *core_name);

#endif
