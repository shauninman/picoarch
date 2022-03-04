#include "overrides.h"

static const struct core_override_option gambatte_core_option_overrides[] = {
	{
		.key = "gambatte_gb_colorization",
		.default_value = "internal",
		.info = "'Auto' selects the 'best' palette. 'Internal' uses 'Internal Palette' option. 'Custom' loads user palette from system directory.",
	},
	{
		.key = "gambatte_gb_internal_palette",
		.default_value = "TWB64 - Pack 1",
		.info = "Selects palette used when 'GB Colorization' is set to 'Internal.'",
		.options = {
			[50] = { "Special 4", NULL },
		}
	},
	{
		.key = "gambatte_gb_palette_twb64_1",
		.desc = "TWB64 - Pack 1",
		.default_value = "TWB64 038 - Pokemon mini Ver.",
	},
	{
		.key = "gambatte_gb_palette_twb64_2",
		.desc = "TWB64 - Pack 2",
	},
	{
		.key = "gambatte_gbc_color_correction",
		.info = "Adjusts colors to match the display of real GBC hardware. 'GBC Only' only applies correction when playing GBC games or using a GBC palette."
	},
	{
		.key = "gambatte_gbc_color_correction_mode",
		.desc = "CC Mode",
		.info = "'Accurate' produces output almost indistinguishable from a real GBC LCD panel. 'Fast' merely darkens colors and reduces saturation.",
		.default_value = "fast",
	},
	{
		.key = "gambatte_gbc_frontlight_position",
		.desc = "CC Frontlight",
		.info = "Simulates the GBC LCD panel when illuminated from different angles. This setting only applies when 'Color Correction Mode' is set to 'Accurate'."
	},
	{
		.key = "gambatte_dark_filter_level",
		.desc = "Dark Level",
		.info = "Enable luminosity-based brightness reduction. May be used to avoid glare/eye strain when playing games with white backgrounds."
	},
	{
		.key = "gambatte_gb_hwmode",
		.desc = "Hardware (restart)",
		.info = "Which type of hardware to emulate. 'Auto' is recommended. 'GBA' unlocks extra features in certain 'GBA enhanced' Game Boy Color games."
	},
	{
		.key = "gambatte_gb_bootloader",
		.desc = "Boot Logo (restart)",
		.default_value = "disabled",
	},
	{
		.key = "gambatte_mix_frames",
		.desc = "Blending",
		.info = "Simulates LCD ghosting effects. 'Simple' performs a 50:50 mix. 'Ghosting' mimics LCD response times with multiple buffered frames.",
		.options = {
			[2] = { "lcd_ghosting",         "Ghosting (Accurate)" },
			[3] = { "lcd_ghosting_fast",    "Ghosting (Fast)"     },
			[4] = { "lcd_ghosting_fastest", "Ghosting (Fastest)"  },
		}
	},
	{
		.key = "gambatte_up_down_allowed",
		.desc = "Opp. Directions",
		.info = "Enabling this will allow pressing / quickly alternating / holding both left and right (or up and down) directions at the same time."
	},
	// {
	// 	.key = "gambatte_rumble_level",
	// 	.blocked = true
	// },
	{
		.key = "gambatte_show_gb_link_settings",
		.blocked = true
	},
	{ NULL }
};

me_bind_action gambatte_ctrl_actions[] =
{
	{ "UP       ",  1 << RETRO_DEVICE_ID_JOYPAD_UP},
	{ "DOWN     ",  1 << RETRO_DEVICE_ID_JOYPAD_DOWN },
	{ "LEFT     ",  1 << RETRO_DEVICE_ID_JOYPAD_LEFT },
	{ "RIGHT    ",  1 << RETRO_DEVICE_ID_JOYPAD_RIGHT },
	{ "A BUTTON ",  1 << RETRO_DEVICE_ID_JOYPAD_A },
	{ "B BUTTON ",  1 << RETRO_DEVICE_ID_JOYPAD_B },
	{ "A TURBO  ",  1 << RETRO_DEVICE_ID_JOYPAD_X },
	{ "B TURBO  ",  1 << RETRO_DEVICE_ID_JOYPAD_Y },
	{ "START    ",  1 << RETRO_DEVICE_ID_JOYPAD_START },
	{ "SELECT   ",  1 << RETRO_DEVICE_ID_JOYPAD_SELECT },
	{ NULL,       0 }
};

#define gambatte_overrides {                                    \
	.core_name = "gambatte",                                \
	.actions = gambatte_ctrl_actions,                       \
	.action_size = array_size(gambatte_ctrl_actions),       \
	.options = gambatte_core_option_overrides               \
}
