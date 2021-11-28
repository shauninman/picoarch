#include "overrides.h"

static const struct core_override_option bluemsx_core_option_overrides[] = {
	{
		.key = "bluemsx_msxtype",
		.desc = "Machine (Restart)",
		.options = {
			[8] = { "SVI-318", NULL },
			[9] = { "SVI-328", NULL },
			[10] = { "SVI-328 MK2", NULL },
			[12] = { "SVI-603", NULL },
		},
	},
	{
		.key = "bluemsx_vdp_synctype",
		.desc = "VDP Sync (Restart)",
	},
	{
		.key = "bluemsx_ym2413_enable",
		.desc = "YM2413 (Restart)",
		.info = "Enable YM2413 sound",
	},
	{
		.key = "bluemsx_cartmapper",
		.desc = "Mapper (Restart)",
		.info = "Manually select the correct mapper when a cart is not yet in the database.",
	},
	{
		.key = "bluemsx_auto_rewind_cas",
		.desc = "Auto Rewind",
	},
	{ NULL }
};

me_bind_action bluemsx_ctrl_actions[] =
{
	{ "UP       ",  1 << RETRO_DEVICE_ID_JOYPAD_UP},
	{ "DOWN     ",  1 << RETRO_DEVICE_ID_JOYPAD_DOWN },
	{ "LEFT     ",  1 << RETRO_DEVICE_ID_JOYPAD_LEFT },
	{ "RIGHT    ",  1 << RETRO_DEVICE_ID_JOYPAD_RIGHT },
	{ "BTN 1    ",  1 << RETRO_DEVICE_ID_JOYPAD_A },
	{ "BTN 2    ",  1 << RETRO_DEVICE_ID_JOYPAD_B },
	{ "BTN 3    ",  1 << RETRO_DEVICE_ID_JOYPAD_Y },
	{ "BTN 4    ",  1 << RETRO_DEVICE_ID_JOYPAD_X },
	{ "BTN 5    ",  1 << RETRO_DEVICE_ID_JOYPAD_L },
	{ "BTN 6    ",  1 << RETRO_DEVICE_ID_JOYPAD_R },
	{ "COL 5    ",  1 << RETRO_DEVICE_ID_JOYPAD_R2 },
	{ "COL 6    ",  1 << RETRO_DEVICE_ID_JOYPAD_L2 },
	{ "COL #    ",  1 << RETRO_DEVICE_ID_JOYPAD_START },
	{ "COL *    ",  1 << RETRO_DEVICE_ID_JOYPAD_SELECT },
	{ NULL,       0 }
};

#define bluemsx_overrides {                         \
	.core_name = "bluemsx",                           \
	.actions = bluemsx_ctrl_actions,                  \
	.action_size = array_size(bluemsx_ctrl_actions),  \
	.options = bluemsx_core_option_overrides          \
}
