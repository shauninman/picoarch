#include "overrides.h"

me_bind_action nxengine_ctrl_actions[] =
{
	{ "UP         ",  1 << RETRO_DEVICE_ID_JOYPAD_UP},
	{ "DOWN       ",  1 << RETRO_DEVICE_ID_JOYPAD_DOWN },
	{ "LEFT       ",  1 << RETRO_DEVICE_ID_JOYPAD_LEFT },
	{ "RIGHT      ",  1 << RETRO_DEVICE_ID_JOYPAD_RIGHT },
	{ "FIRE       ",  1 << RETRO_DEVICE_ID_JOYPAD_A },
	{ "JUMP       ",  1 << RETRO_DEVICE_ID_JOYPAD_B },
	{ "MAP        ",  1 << RETRO_DEVICE_ID_JOYPAD_X },
	{ "INVENTORY  ",  1 << RETRO_DEVICE_ID_JOYPAD_START },
	{ "SETTINGS   ",  1 << RETRO_DEVICE_ID_JOYPAD_SELECT },
	{ "PREV WEAPON",  1 << RETRO_DEVICE_ID_JOYPAD_L },
	{ "NEXT WEAPON",  1 << RETRO_DEVICE_ID_JOYPAD_R },
	{ NULL,       0 }
};

#define nxengine_overrides {                               \
	.core_name = "nxengine",                               \
	.actions = nxengine_ctrl_actions,                      \
	.action_size = array_size(nxengine_ctrl_actions),      \
}

