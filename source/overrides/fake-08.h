#include "overrides.h"

static const struct core_override_option fake_08_core_option_overrides[] = {	//
	// {
	// 	.key = "fceumm_zapper_tolerance",
	// 	.blocked = true
	// },
	{ NULL }
};

me_bind_action fake_08_ctrl_actions[] =
{
	{ "UP       ",  1 << RETRO_DEVICE_ID_JOYPAD_UP},
	{ "DOWN     ",  1 << RETRO_DEVICE_ID_JOYPAD_DOWN },
	{ "LEFT     ",  1 << RETRO_DEVICE_ID_JOYPAD_LEFT },
	{ "RIGHT    ",  1 << RETRO_DEVICE_ID_JOYPAD_RIGHT },
	{ "A        ",  1 << RETRO_DEVICE_ID_JOYPAD_A },
	{ "B        ",  1 << RETRO_DEVICE_ID_JOYPAD_B },
	{ "START    ",  1 << RETRO_DEVICE_ID_JOYPAD_START },
	{ NULL,       0 }
};

#define fake_08_overrides {                               \
	.core_name = "fake-08",                               \
	.actions = fake_08_ctrl_actions,                      \
	.action_size = array_size(fake_08_ctrl_actions),      \
	.options = fake_08_core_option_overrides              \
}
