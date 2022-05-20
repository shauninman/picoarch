#include "overrides.h"

static const struct core_override_option gpsp_core_option_overrides[] = {
	{
		.key = "gpsp_bios",
		.info = "Choose the BIOS image to use. The official BIOS must be provided by the user. Using a builtin BIOS might result in compatibility problems.",
		.options = {
			{ "auto", "auto" },
			{ "builtin", "builtin" },
			{ "official", "original" }
		}
	},
	{
		.key = "gpsp_boot_mode",
		.info = "Choose whether to boot the BIOS before the game or not.",
		.options = {
			{ "game", "auto" },
			{ "bios", "bios" }
		}
	},
	{
		.key = "gpsp_frameskip",
		.info = "Skip frames to avoid audio crackling. Improves performance at the expense of visual smoothness.",
		.default_value = "disabled",
		.options = {
			[2] = {"auto_threshold", "Threshold"},
			[3] = {"fixed_interval", "Fixed"},
		}
	},
	{
		.key = "gpsp_frameskip_threshold",
		.desc = "FS Threshold (%)",
		.info = "When 'Frameskip' is set to 'Threshold', sets how low the audio buffer can get before frames will be skipped.",
	},
	{
		.key = "gpsp_frameskip_interval",
		.desc = "FS Interval",
		.info = "The maximum number of frames that can be skipped before a new frame is rendered.",
		.default_value = "3"
	},
	{
		.key = "gpsp_color_correction",
		.info = "Adjusts output colors to match real GBA hardware.",
		.options = {
			{"disabled", NULL},
			{"enabled", NULL},
			{ NULL, NULL }
		}
	},
	{
		.key = "gpsp_frame_mixing",
		.desc = "Frame Blending",
		.info = "Simulates LCD ghosting effects.",
		.options = {
			{"disabled", NULL},
			{"enabled", NULL},
			{ NULL, NULL }
		}
	},
	{
		.key = "gpsp_save_method",
		.default_value = "libretro",
		.blocked = true
	},
	{ NULL }
};

me_bind_action gpsp_ctrl_actions[] =
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

const struct core_override_fast_forward gpsp_fast_forward = {
	.type_key = "gpsp_frameskip",
	.interval_key = "gpsp_frameskip_interval"
};

#define gpsp_overrides {                                \
	.core_name = "gpsp",                            \
	.fast_forward = &gpsp_fast_forward,             \
	.actions = gpsp_ctrl_actions,                   \
	.action_size = array_size(gpsp_ctrl_actions),   \
	.options = gpsp_core_option_overrides           \
}
