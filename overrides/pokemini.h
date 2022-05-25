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

me_bind_action pokemini_ctrl_actions[] =
{
	{ "UP       ",  1 << RETRO_DEVICE_ID_JOYPAD_UP},
	{ "DOWN     ",  1 << RETRO_DEVICE_ID_JOYPAD_DOWN },
	{ "LEFT     ",  1 << RETRO_DEVICE_ID_JOYPAD_LEFT },
	{ "RIGHT    ",  1 << RETRO_DEVICE_ID_JOYPAD_RIGHT },
	{ "A BUTTON ",  1 << RETRO_DEVICE_ID_JOYPAD_A },
	{ "B BUTTON ",  1 << RETRO_DEVICE_ID_JOYPAD_B },
	{ "C BUTTON ",  1 << RETRO_DEVICE_ID_JOYPAD_R },
	{ "SHAKE    ",  1 << RETRO_DEVICE_ID_JOYPAD_L },
	{ "POWER    ",  1 << RETRO_DEVICE_ID_JOYPAD_SELECT },
	{ NULL,       0 }
};

#define pokemini_overrides {                               \
	.core_name = "pokemini",                               \
	.actions = pokemini_ctrl_actions,                      \
	.action_size = array_size(pokemini_ctrl_actions),      \
	.options = pokemini_core_option_overrides              \
}

