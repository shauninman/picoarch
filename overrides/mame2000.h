#include "overrides.h"

static const struct core_override_option mame2000_core_option_overrides[] = {
	{
		.key = "mame2000-frameskip",
		.info = "Skip frames to avoid audio crackling. Improves performance at the expense of visual smoothness.",
		.default_value = "auto",
		.retro_var_value = "Frameskip ; disabled|auto|threshold"
	},
	{
		.key = "mame2000-frameskip_threshold",
		.info = "When 'Frameskip' is set to 'Threshold', sets how low the audio buffer can get before frames will be skipped.",
		.retro_var_value = "FS Threshold (%); 30|40|50|60",
	},
	{
		.key = "mame2000-frameskip_interval",
		.info = "The maximum number of frames that can be skipped before a new frame is rendered.",
		.default_value = "2",
		.retro_var_value = "FS Interval; 1|2|3|4|5|6|7|8|9",
	},
	{
		.key = "mame2000-skip_disclaimer",
		.default_value = "enabled",
		.retro_var_value = "Skip Disclaimer; disabled|enabled"
	},
	{
		.key = "mame2000-show_gameinfo",
		.retro_var_value = "Show Game Info; disabled|enabled"
	},
	{
		.key = "mame2000-sample_rate",
		.default_value = "22050",
		.retro_var_value = "Snd. Rate (Restart); 11025|22050|32000|44100"
	},
	{
		.key = "mame2000-stereo",
		.retro_var_value = "Stereo (Restart); disabled|enabled"
	},
	{ NULL }
};

me_bind_action mame2000_ctrl_actions[] =
{
	{ "UP       ",  1 << RETRO_DEVICE_ID_JOYPAD_UP},
	{ "DOWN     ",  1 << RETRO_DEVICE_ID_JOYPAD_DOWN },
	{ "LEFT     ",  1 << RETRO_DEVICE_ID_JOYPAD_LEFT },
	{ "RIGHT    ",  1 << RETRO_DEVICE_ID_JOYPAD_RIGHT },
	{ "BUTTON 1 ",  1 << RETRO_DEVICE_ID_JOYPAD_B },
	{ "BUTTON 2 ",  1 << RETRO_DEVICE_ID_JOYPAD_A },
	{ "BUTTON 3 ",  1 << RETRO_DEVICE_ID_JOYPAD_Y },
	{ "BUTTON 4 ",  1 << RETRO_DEVICE_ID_JOYPAD_X },
	{ "BUTTON 5 ",  1 << RETRO_DEVICE_ID_JOYPAD_L },
	{ "BUTTON 6 ",  1 << RETRO_DEVICE_ID_JOYPAD_R },
	{ "START    ",  1 << RETRO_DEVICE_ID_JOYPAD_START },
	{ "COIN     ",  1 << RETRO_DEVICE_ID_JOYPAD_SELECT },
	{ "OSD MENU ",  1 << RETRO_DEVICE_ID_JOYPAD_R2 },
	{ NULL,       0 }
};

const struct core_override_fast_forward mame2000_fast_forward = {
	.type_key = "mame2000-frameskip",
	.type_value = "auto",
	.interval_key = "mame2000-frameskip_interval"
};

#define mame2000_overrides {                              \
	.core_name = "mame2000",                          \
	.fast_forward = &mame2000_fast_forward,           \
	.actions = mame2000_ctrl_actions,                 \
	.action_size = array_size(mame2000_ctrl_actions), \
	.options = mame2000_core_option_overrides,        \
	.block_load_content = 1                           \
}
