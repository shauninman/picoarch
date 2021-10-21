#include "overrides.h"

static const struct core_override_option quicknes_core_option_overrides[] = {
	{
		.key = "quicknes_use_overscan_h",
		.desc = "Horiz. Overscan",
		.info = "When disabled, crop out (horizontally) the potentially random glitchy video output that would have been hidden by the TV screen bezel.",
	},
	{
		.key = "quicknes_use_overscan_v",
		.desc = "Vert. Overscan",
		.info = "When disabled, crop out (vertically) the potentially random glitchy video output that would have been hidden by the TV screen bezel.",
	},
	{
		.key = "quicknes_palette",
		.desc = "Palette",
		.options = {
			{ "default",              "Default" },
			{ "asqrealc",             "ASQ's Real" },
			{ "nintendo-vc",          "Nintendo VC" },
			{ "rgb",                  "Nintendo RGB PPU" },
			{ "yuv-v3",               "FBX's YUV-V3" },
			{ "unsaturated-final",    "FBX's Unsaturated" },
			{ "sony-cxa2025as-us",    "Sony CXA2025AS US" },
			{ "pal",                  "PAL" },
			{ "bmf-final2",           "BMF's Final 2" },
			{ "bmf-final3",           "BMF's Final 3" },
			{ "smooth-fbx",           "FBX's Smooth" },
			{ "composite-direct-fbx", "FBX's Compos. Direct" },
			{ "pvm-style-d93-fbx",    "FBX's PVM Style D93" },
			{ "ntsc-hardware-fbx",    "FBX's NTSC Hardware" },
			{ "nes-classic-fbx-fs",   "FBX's NES-Classic FS" },
			{ "nescap",               "RGBSource's NESCAP" },
			{ "wavebeam",             "nakedarthur's Wavebeam" },
			{ NULL, NULL},
		},
	},
	{
		.key = "quicknes_no_sprite_limit",
		.info = "Removes the 8-sprite-per-scanline hardware limit. Reduces flickering at the risk of visual glitches.",
	},
	{
		.key = "quicknes_audio_eq",
		.desc = "Audio EQ",
	},
	{
		.key = "quicknes_audio_nonlinear",
		.info = "'Non-Linear' simulates the NES APU. 'Linear' is less accurate but faster. 'Stereo Panning' adds depth with panning techniques and reverb.",
	},
	{
		.key = "quicknes_turbo_pulse_width",
		.desc = "Turbo (in frames)",
		.info = "Specifies on / off frame count when turbo buttons are held."
	},
	{
		.key = "quicknes_up_down_allowed",
		.desc = "Allow Opp. Input",
		.info = "Enabling allows pressing both left and right (or up and down) directions at the same time. May cause glitches."
	},
	{
		.key = "quicknes_aspect_ratio_par",
		.blocked = true
	},
	{ NULL }
};

me_bind_action quicknes_ctrl_actions[] =
{
	{ "UP       ",  1 << RETRO_DEVICE_ID_JOYPAD_UP},
	{ "DOWN     ",  1 << RETRO_DEVICE_ID_JOYPAD_DOWN },
	{ "LEFT     ",  1 << RETRO_DEVICE_ID_JOYPAD_LEFT },
	{ "RIGHT    ",  1 << RETRO_DEVICE_ID_JOYPAD_RIGHT },
	{ "A        ",  1 << RETRO_DEVICE_ID_JOYPAD_A },
	{ "B        ",  1 << RETRO_DEVICE_ID_JOYPAD_B },
	{ "A TURBO  ",  1 << RETRO_DEVICE_ID_JOYPAD_X },
	{ "B TURBO  ",  1 << RETRO_DEVICE_ID_JOYPAD_Y },
	{ "START    ",  1 << RETRO_DEVICE_ID_JOYPAD_START },
	{ "SELECT   ",  1 << RETRO_DEVICE_ID_JOYPAD_SELECT },
	{ NULL,       0 }
};

#define quicknes_overrides {                                    \
	.core_name = "quicknes",                                \
	.actions = quicknes_ctrl_actions,                       \
	.action_size = array_size(quicknes_ctrl_actions),       \
	.options = quicknes_core_option_overrides               \
}
