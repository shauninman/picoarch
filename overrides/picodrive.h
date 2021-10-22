#include "overrides.h"

static const struct core_override_option picodrive_core_option_overrides[] = {
	{
		.key = "picodrive_overscan",
		.info = "Crop out the potentially random glitchy video output that would have been hidden by the TV bezel."
	},
	{
		.key = "picodrive_audio_filter",
		.info = "Enable a low pass audio filter to better simulate the characteristic sound of a Model 1 Genesis. Ignored for SMS and PICO.",
	},
	{
		.key = "picodrive_lowpass_range",
		.info = "Specify the cut-off frequency of the audio low pass filter. A higher value increases the perceived 'strength' of the filter.",
	},
	{
		.key = "picodrive_frameskip",
		.info = "Skip frames to avoid audio crackling. Improves performance at the expense of visual smoothness.",
	},
	{
		.key = "picodrive_frameskip_threshold",
		.desc = "FS Threshold (%)",
		.info = "When 'Frameskip' is set to 'Threshold', sets how low the audio buffer can get before frames will be skipped.",
	},
	{
		.key = "picodrive_aspect",
		.blocked = true,
	},
	{ NULL }
};

me_bind_action picodrive_ctrl_actions[] =
{
	{ "UP       ",  1 << RETRO_DEVICE_ID_JOYPAD_UP},
	{ "DOWN     ",  1 << RETRO_DEVICE_ID_JOYPAD_DOWN },
	{ "LEFT     ",  1 << RETRO_DEVICE_ID_JOYPAD_LEFT },
	{ "RIGHT    ",  1 << RETRO_DEVICE_ID_JOYPAD_RIGHT },
	{ "A        ",  1 << RETRO_DEVICE_ID_JOYPAD_Y },
	{ "B        ",  1 << RETRO_DEVICE_ID_JOYPAD_B },
	{ "C        ",  1 << RETRO_DEVICE_ID_JOYPAD_A },
	{ "X        ",  1 << RETRO_DEVICE_ID_JOYPAD_L },
	{ "Y        ",  1 << RETRO_DEVICE_ID_JOYPAD_X },
	{ "Z        ",  1 << RETRO_DEVICE_ID_JOYPAD_R },
	{ "MODE     ",  1 << RETRO_DEVICE_ID_JOYPAD_SELECT },
	{ "START    ",  1 << RETRO_DEVICE_ID_JOYPAD_START },
	{ NULL,       0 }
};

const struct core_override_fast_forward picodrive_fast_forward = {
	.type_key = "picodrive_frameskip",
	.type_value = "auto",
	.interval_key = "picodrive_frameskip_interval",
};

#define picodrive_overrides {                                    \
	.core_name = "picodrive",                                \
	.fast_forward = &picodrive_fast_forward,                 \
	.actions = picodrive_ctrl_actions,                       \
	.action_size = array_size(picodrive_ctrl_actions),       \
	.options = picodrive_core_option_overrides               \
}
