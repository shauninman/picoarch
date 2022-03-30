#include "overrides.h"

static const struct core_override_option smsplus_core_option_overrides[] = {
	{
		.key = "smsplus_sms_bios",
		.desc = "SMS BIOS (Restart)",
		.info = "Use official BIOS/bootloader, if present in the system directory.",
	},
	{
		.key = "smsplus_hardware",
		.desc = "Hardware (Restart)",
		.options = {
			{ "auto",             NULL     },
			{ "master system",    "sms"    },
			{ "master system II", "sms II" },
			{ "game gear",        NULL     },
			{ "game gear (sms compatibility)",  "gg (sms compat.)" },
			{ "coleco",           NULL     },
			{ NULL,               NULL     },
		},
	},
	{
		.key = "smsplus_region",
		.desc = "Region (Restart)",
	},
	{
		.key = "smsplus_fm_sound",
		.desc = "Use FM (Restart)",
		.info = "Use FM Sound for some Master System games to enable enhanced music. Some games need Region set to ntsc-j (e.g. Wonder Boy III). Needs restart.",
	},
	{
		.key = "smsplus_hide_left_border",
		.desc = "Hide SMS Border",
	},
	{ NULL }
};

me_bind_action smsplus_ctrl_actions[] =
{
	{ "UP       ",  1 << RETRO_DEVICE_ID_JOYPAD_UP},
	{ "DOWN     ",  1 << RETRO_DEVICE_ID_JOYPAD_DOWN },
	{ "LEFT     ",  1 << RETRO_DEVICE_ID_JOYPAD_LEFT },
	{ "RIGHT    ",  1 << RETRO_DEVICE_ID_JOYPAD_RIGHT },
	{ "BTN 1    ",  1 << RETRO_DEVICE_ID_JOYPAD_A },
	{ "BTN 2    ",  1 << RETRO_DEVICE_ID_JOYPAD_B },
	{ "COLECO 1 ",  1 << RETRO_DEVICE_ID_JOYPAD_X },
	{ "COLECO 2 ",  1 << RETRO_DEVICE_ID_JOYPAD_Y },
	{ "START / #",  1 << RETRO_DEVICE_ID_JOYPAD_START },
	{ "COLECO * ",  1 << RETRO_DEVICE_ID_JOYPAD_SELECT },
	{ "COLECO 3 ",  1 << RETRO_DEVICE_ID_JOYPAD_R },
	{ "COLECO 4 ",  1 << RETRO_DEVICE_ID_JOYPAD_L },
	{ "COLECO 5 ",  1 << RETRO_DEVICE_ID_JOYPAD_R2 },
	{ "COLECO 6 ",  1 << RETRO_DEVICE_ID_JOYPAD_L2 },
	{ NULL,       0 }
};

#define smsplus_overrides {                                    \
	.core_name = "smsplus-gx",                             \
	.actions = smsplus_ctrl_actions,                       \
	.action_size = array_size(smsplus_ctrl_actions),       \
	.options = smsplus_core_option_overrides               \
}
