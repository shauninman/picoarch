#include "overrides.h"

static const struct core_override_option pcsx_rearmed_core_option_overrides[] = {
	{
		.key = "pcsx_rearmed_frameskip_type",
		.info = "Skip frames to avoid audio crackling. Improves performance at the expense of visual smoothness. Will cause graphical glitches.",
		.default_value = "disabled",
		.options = {
			[2] = {"auto_threshold", "Threshold"},
			[3] = {"fixed_interval", "Fixed"},
		}
	},
	{
		.key = "pcsx_rearmed_frameskip_threshold",
		.desc = "FS Threshold (%)",
		.info = "When 'Frameskip' is set to 'Threshold', sets how low the audio buffer can get before frames will be skipped.",
	},
	{
		.key = "pcsx_rearmed_frameskip",
		.desc = "FS Interval",
		.info = "Specifies the maximum number of frames that can be skipped before a new frame is rendered.",
		.default_value = "4"
	},
	{
		.key = "pcsx_rearmed_memcard2",
		.desc = "2nd Memory Card",
	},
	{
		.key = "pcsx_rearmed_dithering",
		.desc = "Dithering",
		.info = "If disabled, turns off the dithering pattern the PSX applies to combat color banding.",
		.default_value = "enabled",
	},
	
	// TODO: don't apply to TRIMUI

	{
		.key = "pcsx_rearmed_neon_interlace_enable",
		.default_value = "disabled",
		.blocked = true
	},
	{
		.key = "pcsx_rearmed_neon_enhancement_enable",
		.default_value = "disabled",
		.blocked = true
	},
	{
		.key = "pcsx_rearmed_neon_enhancement_no_main",
		.default_value = "disabled",
		.blocked = true
	},
	
	// TODO: don't apply to MIYOOMINI
	
	{
		.key = "pcsx_rearmed_show_gpu_peops_settings",
		.desc = "P.E.Op.S. Settings",
	},
	{
		.key = "pcsx_rearmed_gpu_peops_odd_even_bit",
		.desc = "Odd/Even Hack",
	},
	{
		.key = "pcsx_rearmed_gpu_peops_expand_screen_width",
		.desc = "Exp. Screen Width",
	},
	{
		.key = "pcsx_rearmed_gpu_peops_ignore_brightness",
		.desc = "Ignore Brightness",
	},
	{
		.key = "pcsx_rearmed_gpu_peops_disable_coord_check",
		.desc = "Coordinate Check",
	},
	{
		.key = "pcsx_rearmed_gpu_peops_lazy_screen_update",
		.desc = "Lazy Screen Update",
	},
	{
		.key = "pcsx_rearmed_gpu_peops_old_frame_skip",
		.desc = "Old Frameskip",
	},
	{
		.key = "pcsx_rearmed_gpu_peops_repeated_triangles",
		.desc = "Rep. Triangles",
	},
	{
		.key = "pcsx_rearmed_gpu_peops_quads_with_triangles",
		.desc = "Quads w/ Triangles",
	},
	{
		.key = "pcsx_rearmed_gpu_peops_fake_busy_state",
		.desc = "Fake 'Busy' States",
	},
	{
		.key = "pcsx_rearmed_show_gpu_unai_settings",
		.desc = "GPU UNAI Settings",
		.info = "Shows or hides advanced gpu settings. Menu must be toggled for this setting to take effect."
	},
	{
		.key = "pcsx_rearmed_gpu_unai_blending",
		.desc = "Blending",
	},
	{
		.key = "pcsx_rearmed_gpu_unai_lighting",
		.desc = "Lighting",
	},
	{
		.key = "pcsx_rearmed_gpu_unai_fast_lighting",
		.desc = "Fast Lighting",
	},
	{
		.key = "pcsx_rearmed_gpu_unai_ilace_force",
		.desc = "Forced Interlace",
	},
	{
		.key = "pcsx_rearmed_gpu_unai_pixel_skip",
		.desc = "Pixel Skip",
	},
	{
		.key = "pcsx_rearmed_gpu_unai_scale_hires",
		.desc = "Hi-Res Scaling",
	},
	{
		.key = "pcsx_rearmed_gpu_thread_rendering",
		.info = "Runs GPU commands in a thread. Sync waits for drawing to finish before vsync. Async will not wait unless there's another frame behind it."
	},
	
	{
		.key = "pcsx_rearmed_pe2_fix",
		.desc = "PE 2/VH 1/2 Fix",
		.info = "Fixes Parasite Eve 2 / Vandal Hearts 1 / 2."
	},
	{
		.key = "pcsx_rearmed_inuyasha_fix",
		.desc = "InuYasha Battle Fix",
	},
	{
		.key = "pcsx_rearmed_async_cd",
		.desc = "CD Access (Restart)",
		.info = "Method used to read data from disk. 'Asynchronous' can reduce stuttering on devices with slow storage. 'Precache' loads disk image into memory (CHD only)."
	},
	{
		.key = "pcsx_rearmed_spuirq",
		.desc = "SPU IRQ Always",
	},
	{
		.key = "pcsx_rearmed_nosmccheck",
		.desc = "Disable SMC Checks",
	},
	{
		.key = "pcsx_rearmed_gteregsunneeded",
		.desc = "Unneeded GTE Regs",
	},
	{
		.key = "pcsx_rearmed_nogteflags",
		.desc = "Disable GTE Flags",
	},
	{
		.key = "pcsx_rearmed_show_other_input_settings",
		.blocked = true
	},
	{
		.key = "pcsx_rearmed_input_sensitivity",
		.blocked = true
	},
	{
		.key = "pcsx_rearmed_multitap",
		.blocked = true
	},
	// {
	// 	.key = "pcsx_rearmed_vibration",
	// 	.blocked = true
	// },
	{
		.key = "pcsx_rearmed_display_internal_fps",
		.blocked = true
	},
	{
		.key = "pcsx_rearmed_icache_emulation",
		.blocked = true
	},
	{ NULL }
};

me_bind_action pcsx_rearmed_ctrl_actions[] =
{
	{ "UP       ",  1 << RETRO_DEVICE_ID_JOYPAD_UP},
	{ "DOWN     ",  1 << RETRO_DEVICE_ID_JOYPAD_DOWN },
	{ "LEFT     ",  1 << RETRO_DEVICE_ID_JOYPAD_LEFT },
	{ "RIGHT    ",  1 << RETRO_DEVICE_ID_JOYPAD_RIGHT },
	{ "CIRCLE   ",  1 << RETRO_DEVICE_ID_JOYPAD_A },
	{ "CROSS    ",  1 << RETRO_DEVICE_ID_JOYPAD_B },
	{ "TRIANGLE ",  1 << RETRO_DEVICE_ID_JOYPAD_X },
	{ "SQUARE   ",  1 << RETRO_DEVICE_ID_JOYPAD_Y },
	{ "START    ",  1 << RETRO_DEVICE_ID_JOYPAD_START },
	{ "SELECT   ",  1 << RETRO_DEVICE_ID_JOYPAD_SELECT },
	{ "L1 BUTTON",  1 << RETRO_DEVICE_ID_JOYPAD_L },
	{ "R1 BUTTON",  1 << RETRO_DEVICE_ID_JOYPAD_R },
	{ "L2 BUTTON",  1 << RETRO_DEVICE_ID_JOYPAD_L2 },
	{ "R2 BUTTON",  1 << RETRO_DEVICE_ID_JOYPAD_R2 },
	{ NULL,       0 }
};

const struct core_override_fast_forward pcsx_rearmed_fast_forward = {
	.type_key = "pcsx_rearmed_frameskip_type",
	.interval_key = "pcsx_rearmed_frameskip_interval"
};

#define pcsx_rearmed_overrides {                               \
	.core_name = "pcsx_rearmed",                           \
	.fast_forward = &pcsx_rearmed_fast_forward,            \
	.actions = pcsx_rearmed_ctrl_actions,                  \
	.action_size = array_size(pcsx_rearmed_ctrl_actions),  \
	.options = pcsx_rearmed_core_option_overrides,         \
}

