#include "overrides.h"

me_bind_action gme_ctrl_actions[] =
{
	{ "PREV TRK ",  1 << RETRO_DEVICE_ID_JOYPAD_L },
	{ "NEXT TRK ",  1 << RETRO_DEVICE_ID_JOYPAD_R },
	{ "PAUSE    ",  1 << RETRO_DEVICE_ID_JOYPAD_START },
	{ NULL,       0 }
};

#define gme_overrides {                                    \
	.core_name = "gme",                                \
	.actions = gme_ctrl_actions,                       \
	.action_size = array_size(gme_ctrl_actions),       \
}
