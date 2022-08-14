#include "overrides.h"

static const struct core_override_option mgba_core_option_overrides[] = {
	{
		.key = "mgba_skip_bios",
		.default_value = "ON",
	},
	{ NULL }
};

me_bind_action mgba_ctrl_actions[] =
{
	{ "UP       ",  1 << RETRO_DEVICE_ID_JOYPAD_UP},
	{ "DOWN     ",  1 << RETRO_DEVICE_ID_JOYPAD_DOWN },
	{ "LEFT     ",  1 << RETRO_DEVICE_ID_JOYPAD_LEFT },
	{ "RIGHT    ",  1 << RETRO_DEVICE_ID_JOYPAD_RIGHT },
	{ "A BUTTON ",  1 << RETRO_DEVICE_ID_JOYPAD_A },
	{ "B BUTTON ",  1 << RETRO_DEVICE_ID_JOYPAD_B },
	{ "A TURBO  ",  1 << RETRO_DEVICE_ID_JOYPAD_X },
	{ "B TURBO  ",  1 << RETRO_DEVICE_ID_JOYPAD_Y },
	{ "START    ",  1 << RETRO_DEVICE_ID_JOYPAD_START },
	{ "SELECT   ",  1 << RETRO_DEVICE_ID_JOYPAD_SELECT },
	{ "L BUTTON ",  1 << RETRO_DEVICE_ID_JOYPAD_L },
	{ "R BUTTON ",  1 << RETRO_DEVICE_ID_JOYPAD_R },
	{ NULL,       0 }
};

const struct core_override_fast_forward mgba_fast_forward = {
	.type_key = "mgba_frameskip",
	.interval_key = "mgba_frameskip_interval"
};

#define mgba_overrides {                            \
	.core_name = "mgba",                            \
	.fast_forward = &mgba_fast_forward,             \
	.actions = mgba_ctrl_actions,                   \
	.action_size = array_size(mgba_ctrl_actions),   \
	.options = mgba_core_option_overrides           \
}
