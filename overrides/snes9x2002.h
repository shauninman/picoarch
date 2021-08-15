#include "overrides.h"

static const struct core_override_option snes9x2002_core_option_overrides[] = {
	{
		.key = "snes9x2002_frameskip",
		.info = "Skip frames to avoid audio crackling. Improves performance at the expense of visual smoothness.",
		.default_value = "auto",
		.retro_var_value = "Frameskip ; disabled|auto|threshold"
	},
	{
		.key = "snes9x2002_frameskip_threshold",
		.info = "When 'Frameskip' is set to 'Threshold', sets how low the audio buffer can get before frames will be skipped.",
		.retro_var_value = "FS Threshold (%); 30|40|50|60",
	},
	{
		.key = "snes9x2002_frameskip_interval",
		.info = "The maximum number of frames that can be skipped before a new frame is rendered.",
		.default_value = "4",
		.retro_var_value = "FS Interval; 1|2|3|4|5|6|7|8|9"
	},
	{
		.key = "snes9x2002_overclock_cycles",
		.info = "Hack, unsafe. Requires restart.",
		.retro_var_value = "Overclock (Restart); disabled|compatible|max"
	},
	{ NULL }
};

me_bind_action snes9x2002_ctrl_actions[] =
{
	{ "UP       ",  1 << RETRO_DEVICE_ID_JOYPAD_UP},
	{ "DOWN     ",  1 << RETRO_DEVICE_ID_JOYPAD_DOWN },
	{ "LEFT     ",  1 << RETRO_DEVICE_ID_JOYPAD_LEFT },
	{ "RIGHT    ",  1 << RETRO_DEVICE_ID_JOYPAD_RIGHT },
	{ "A BUTTON ",  1 << RETRO_DEVICE_ID_JOYPAD_A },
	{ "B BUTTON ",  1 << RETRO_DEVICE_ID_JOYPAD_B },
	{ "X BUTTON ",  1 << RETRO_DEVICE_ID_JOYPAD_X },
	{ "Y BUTTON ",  1 << RETRO_DEVICE_ID_JOYPAD_Y },
	{ "START    ",  1 << RETRO_DEVICE_ID_JOYPAD_START },
	{ "SELECT   ",  1 << RETRO_DEVICE_ID_JOYPAD_SELECT },
	{ "L BUTTON ",  1 << RETRO_DEVICE_ID_JOYPAD_L },
	{ "R BUTTON ",  1 << RETRO_DEVICE_ID_JOYPAD_R },
	{ NULL,       0 }
};

const struct core_override_fast_forward snes9x2002_fast_forward = {
	.type_key = "snes9x2002_frameskip",
	.type_value = "auto",
	.interval_key = "snes9x2002_frameskip_interval"
};

#define snes9x2002_overrides {                               \
	.core_name = "snes9x2002",                           \
	.fast_forward = &snes9x2002_fast_forward,            \
	.actions = snes9x2002_ctrl_actions,                  \
	.action_size = array_size(snes9x2002_ctrl_actions),  \
	.options = snes9x2002_core_option_overrides          \
}
