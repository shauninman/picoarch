#include "overrides.h"

static const struct core_override_option pokemini_core_option_overrides[] = {
	{
		.key = "pokemini_video_scale",
		.default_value = "1x",
	},
	{
		.key = "pokemini_palette",
		.default_value = "Old",
	},
	{
		.key = "pokemini_piezofilter",
		.default_value = "disabled",
	},
	{
		.key = "pokemini_lowpass_filter",
		.default_value = "enabled",
	},
	{
		.key = "pokemini_60hz_mode",
		.default_value = "disabled",
	},
	{ NULL }
};

#define pokemini_overrides {                               \
	.core_name = "pokemini",                               \
	.options = pokemini_core_option_overrides              \
}
