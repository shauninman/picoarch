#include "overrides.h"

static const struct core_override_option beetle_pce_fast_core_option_overrides[] = {
	{
		.key = "pce_fast_frameskip",
		.info = "Skip frames to avoid audio crackling. Improves performance at the expense of visual smoothness.",
		.default_value = "auto",
		.options = {
			[2] = {"auto_threshold", "Threshold"},
			[3] = {"fixed_interval", "Fixed"},
		}
	},
	{
		.key = "pce_fast_frameskip_threshold",
		.desc = "FS Threshold (%)",
		.info = "When 'Frameskip' is set to 'Threshold', sets how low the audio buffer can get before frames will be skipped.",
	},
	{
		.key = "pce_fast_frameskip_interval",
		.desc = "FS Interval",
	},
	{
		.key = "pce_hoverscan",
		.desc = "Horiz. Overscan",
		.info = "Modify the horizontal overscan. (352 Width Mode Only)",
	},
	{
		.key = "pce_turbo_toggling",
		.info = "Change III / IV to Toggle Turbo II / Toggle Turbo I",
	},
	{
		.key = "pce_sound_channel_0_volume",
		.desc = "Channel 0 Volume",
	},
	{
		.key = "pce_sound_channel_1_volume",
		.desc = "Channel 1 Volume",
	},
	{
		.key = "pce_sound_channel_2_volume",
		.desc = "Channel 2 Volume",
	},
	{
		.key = "pce_sound_channel_3_volume",
		.desc = "Channel 3 Volume",
	},
	{
		.key = "pce_sound_channel_4_volume",
		.desc = "Channel 4 Volume",
	},
	{
		.key = "pce_sound_channel_5_volume",
		.desc = "Channel 5 Volume",
	},
	{
		.key = "pce_fast_cdimagecache",
		.desc = "CD Cache (Restart)",
	},
	{
		.key = "pce_ocmultiplier",
		.desc = "Overclock (Restart)",
		.info = "Overclock the emulated CPU by selected multiplier.",
	},
	{
		.key = "pce_disable_softreset",
		.desc = "Disable Soft Reset",
	},
	{
		.key = "pce_mouse_sensitivity",
		.blocked = true,
	},
	{
		.key = "pce_turbo_toggle_hotkey",
		.blocked = true,
	},
	{ NULL }
};

me_bind_action beetle_pce_fast_ctrl_actions[] =
{
	{ "UP       ",  1 << RETRO_DEVICE_ID_JOYPAD_UP},
	{ "DOWN     ",  1 << RETRO_DEVICE_ID_JOYPAD_DOWN },
	{ "LEFT     ",  1 << RETRO_DEVICE_ID_JOYPAD_LEFT },
	{ "RIGHT    ",  1 << RETRO_DEVICE_ID_JOYPAD_RIGHT },
	{ "I        ",  1 << RETRO_DEVICE_ID_JOYPAD_A },
	{ "II       ",  1 << RETRO_DEVICE_ID_JOYPAD_B },
	{ "III      ",  1 << RETRO_DEVICE_ID_JOYPAD_Y },
	{ "IV       ",  1 << RETRO_DEVICE_ID_JOYPAD_X },
	{ "V        ",  1 << RETRO_DEVICE_ID_JOYPAD_L },
	{ "VI       ",  1 << RETRO_DEVICE_ID_JOYPAD_R },
	{ "RUN      ",  1 << RETRO_DEVICE_ID_JOYPAD_START },
	{ "SELECT   ",  1 << RETRO_DEVICE_ID_JOYPAD_SELECT },
	{ "2 / 6 BTN",  1 << RETRO_DEVICE_ID_JOYPAD_L2 },
	{ NULL,       0 }
};

const struct core_override_fast_forward beetle_pce_fast_fast_forward = {
	.type_key = "pce_fast_frameskip",
	.type_value = "auto",
	.interval_key = "pce_fast_frameskip_interval",
};

#define beetle_pce_fast_overrides {                                    \
	.core_name = "beetle-pce-fast",                                \
	.fast_forward = &beetle_pce_fast_fast_forward,                 \
	.actions = beetle_pce_fast_ctrl_actions,                       \
	.action_size = array_size(beetle_pce_fast_ctrl_actions),       \
	.options = beetle_pce_fast_core_option_overrides               \
}
