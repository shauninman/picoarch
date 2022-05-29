#include "overrides.h"

static const struct core_override_option fmsx_core_option_overrides[] = {
	{
		.key = "fmsx_hires",
		.retro_var_value = "High res; Off|Interlaced|Progressive"
	},
	{
		.key = "fmsx_mapper_type_mode",
		.retro_var_value = "Mapper Type; "
			"Guess Mapper Type A|"
			"Guess Mapper Type B|"
			"Generic 8kB|"
			"Generic 16kB|"
			"Konami5 8kB|"
			"Konami4 8kB|"
			"ASCII 8kB|"
			"ASCII 16kB|"
			"GameMaster2|"
			"FMPAC"
	},
	{
		.key = "fmsx_simbdos",
		.retro_var_value = "DiskROM calls; Disabled|Enabled"
	},
	{
		.key = "fmsx_autospace",
		.retro_var_value = "Autofire SPACE; No|Yes"
	},
	{
		.key = "fmsx_phantom_disk",
		.retro_var_value = "Make empty disk; No|Yes"
	},
	{
		.key = "fmsx_custom_keyboard_up",
		.blocked = true
	},
	{
		.key = "fmsx_custom_keyboard_down",
		.blocked = true
	},
	{
		.key = "fmsx_custom_keyboard_left",
		.blocked = true
	},
	{
		.key = "fmsx_custom_keyboard_right",
		.blocked = true
	},
	{
		.key = "fmsx_custom_keyboard_a",
		.blocked = true
	},
	{
		.key = "fmsx_custom_keyboard_b",
		.blocked = true
	},
	{
		.key = "fmsx_custom_keyboard_y",
		.blocked = true
	},
	{
		.key = "fmsx_custom_keyboard_x",
		.blocked = true
	},
	{
		.key = "fmsx_custom_keyboard_start",
		.blocked = true
	},
	{
		.key = "fmsx_custom_keyboard_select",
		.blocked = true
	},
	{
		.key = "fmsx_custom_keyboard_l",
		.blocked = true
	},
	{
		.key = "fmsx_custom_keyboard_r",
		.blocked = true
	},
	{
		.key = "fmsx_custom_keyboard_l2",
		.blocked = true
	},
	{
		.key = "fmsx_custom_keyboard_r2",
		.blocked = true
	},
	{
		.key = "fmsx_custom_keyboard_l3",
		.blocked = true
	},
	{
		.key = "fmsx_custom_keyboard_r3",
		.blocked = true
	},
	{ NULL }
};

me_bind_action fmsx_ctrl_actions[] =
{
	{ "UP       ",  1 << RETRO_DEVICE_ID_JOYPAD_UP},
	{ "DOWN     ",  1 << RETRO_DEVICE_ID_JOYPAD_DOWN },
	{ "LEFT     ",  1 << RETRO_DEVICE_ID_JOYPAD_LEFT },
	{ "RIGHT    ",  1 << RETRO_DEVICE_ID_JOYPAD_RIGHT },
	{ "A        ",  1 << RETRO_DEVICE_ID_JOYPAD_A },
	{ "B        ",  1 << RETRO_DEVICE_ID_JOYPAD_B },
	{ "SPACE    ",  1 << RETRO_DEVICE_ID_JOYPAD_Y },
	{ "F1       ",  1 << RETRO_DEVICE_ID_JOYPAD_START },
	{ "F2       ",  1 << RETRO_DEVICE_ID_JOYPAD_SELECT },
	{ "F3       ",  1 << RETRO_DEVICE_ID_JOYPAD_X },
	{ "F4       ",  1 << RETRO_DEVICE_ID_JOYPAD_L },
	{ "F5       ",  1 << RETRO_DEVICE_ID_JOYPAD_R },
	{ "GRAPH    ",  1 << RETRO_DEVICE_ID_JOYPAD_L2 },
	{ "CTRL     ",  1 << RETRO_DEVICE_ID_JOYPAD_R2 },
	{ NULL,       0 }
};

#define fmsx_overrides {                              \
	.core_name = "fmsx",                                \
	.actions = fmsx_ctrl_actions,                       \
	.action_size = array_size(fmsx_ctrl_actions),       \
	.options = fmsx_core_option_overrides               \
}
