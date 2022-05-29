#include "overrides.h"

static const struct core_override_option supafaust_core_option_overrides[] = {
	{
		.key = "supafaust_frameskip",
		.default_value = "auto",
	},
	{
		.key = "supafaust_frameskip_threshold",
		.default_value = "33",
	},
	{
		.key = "supafaust_frameskip_interval",
		.default_value = "4",
	},
	{
		.key = "supafaust_pixel_format",
		.default_value = "rgb565",
		.blocked = true,
	},
	{
		.key = "supafaust_correct_aspect",
		.retro_var_value = "Pixel aspect ratio; disabled|enabled|force_ntsc|force_pal",
		.default_value = "disabled",
		.info = "Requires restart.",
	},
	{
		.key = "supafaust_h_filter",
		.retro_var_value = "Line doubling; none|phr256blend_auto512|phr256blend_512|512_blend|512|phr256blend",
		.default_value = "phr256blend_auto512",
	},
	{
		.key = "supafaust_slstart",
		.retro_var_value = "First line (NTSC); 0|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15|16",
	},
	{
		.key = "supafaust_slend",
		.retro_var_value = "Last line (NTSC); 223|222|221|220|219|218|217|216|215|214|213|212|211|210|209|208|207",
	},
	{
		.key = "supafaust_slstartp",
		.retro_var_value = "First line (PAL); 0|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15|16|17|18|19|20|21|22|23|24",
	},
	{
		.key = "supafaust_slendp",
		.retro_var_value = "Last line (PAL); 238|237|236|235|234|233|232|231|230|229|228|227|226|225|224|223|222|221|220|219|218|217|216|215|214",
	},
	{
		.key = "supafaust_region",
		.retro_var_value = "Region; auto|ntsc|pal|ntsc_lie_auto|pal_lie_auto|ntsc_lie_pal|pal_lie_ntsc",
	},
	{
		.key = "supafaust_frame_begin_vblank",
		.retro_var_value = "Reduce latency; enabled|disabled",
		.info = "Begins frame in SNES VBlank.",
	},
	{
		.key = "supafaust_run_ahead",
		.retro_var_value = "Run-ahead; disabled|video|video+audio",
	},
	{
		.key = "supafaust_multitap",
		.retro_var_value = "Multitap; disabled|port1|port2|port1+port2",
	},
	{
		.key = "supafaust_cx4_clock_rate",
		.retro_var_value = "CX4 clock (%); 100|125|150|175|200|250|300|400|500",
	},
	{
		.key = "supafaust_superfx_clock_rate",
		.retro_var_value = "SuperFX clock (%); 100|125|150|175|200|250|300|400|500|95",
	},
	{
		.key = "supafaust_superfx_icache",
		.retro_var_value = "SuperFX icache; disabled|enabled",
	},
	{
		.key = "supafaust_audio_rate",
		.default_value = "disabled",
		.blocked = true,
	},
	{
		.key = "supafaust_renderer",
		.default_value = "mt",
		.blocked = true,
	},
	{
		.key = "supafaust_thread_affinity_emu",
		.blocked = true,
	},
	{
		.key = "supafaust_thread_affinity_ppu",
		.blocked = true,
	},
	{ NULL }
};

me_bind_action supafaust_ctrl_actions[] =
{
	{ "UP       ",  1 << RETRO_DEVICE_ID_JOYPAD_UP},
	{ "DOWN     ",  1 << RETRO_DEVICE_ID_JOYPAD_DOWN },
	{ "LEFT     ",  1 << RETRO_DEVICE_ID_JOYPAD_LEFT },
	{ "RIGHT    ",  1 << RETRO_DEVICE_ID_JOYPAD_RIGHT },
	{ "A BUTTON ",  1 << RETRO_DEVICE_ID_JOYPAD_A },
	{ "B BUTTON ",  1 << RETRO_DEVICE_ID_JOYPAD_B },
	{ "X BUTTON ",  1 << RETRO_DEVICE_ID_JOYPAD_X },
	{ "Y BUTTON ",  1 << RETRO_DEVICE_ID_JOYPAD_Y },
	{ "START    ",  1 << RETRO_DEVICE_ID_JOYPAD_START },
	{ "SELECT   ",  1 << RETRO_DEVICE_ID_JOYPAD_SELECT },
	{ "L BUTTON ",  1 << RETRO_DEVICE_ID_JOYPAD_L },
	{ "R BUTTON ",  1 << RETRO_DEVICE_ID_JOYPAD_R },
	{ NULL,       0 }
};

const struct core_override_fast_forward supafaust_fast_forward = {
	.type_key = "supafaust_frameskip",
	.type_value = "auto",
	.interval_key = "supafaust_frameskip_interval"
};

#define supafaust_overrides {                          \
	.core_name = "supafaust",                          \
	.fast_forward = &supafaust_fast_forward,           \
	.actions = supafaust_ctrl_actions,                 \
	.action_size = array_size(supafaust_ctrl_actions), \
	.options = supafaust_core_option_overrides         \
}
