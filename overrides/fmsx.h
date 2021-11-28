#include "overrides.h"

static const struct core_override_option fmsx_core_option_overrides[] = {
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
