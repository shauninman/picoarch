#include "overrides.h"
#include "overrides/beetle-pce-fast.h"
#include "overrides/fceumm.h"
#include "overrides/gambatte.h"
#include "overrides/gme.h"
#include "overrides/gpsp.h"
#include "overrides/mame2000.h"
#include "overrides/pcsx_rearmed.h"
#include "overrides/quicknes.h"
#include "overrides/snes9x2002.h"
#include "overrides/snes9x2005.h"
#include "util.h"

static const struct core_override overrides[] = {
	beetle_pce_fast_overrides,
	fceumm_overrides,
	gambatte_overrides,
	gme_overrides,
	gpsp_overrides,
	mame2000_overrides,
	pcsx_rearmed_overrides,
	quicknes_overrides,
	snes9x2002_overrides,
	snes9x2005_overrides,
};

static const struct core_override *override;

const struct core_override *get_overrides(void) {
	return override;
}

void set_overrides(const char *core_name) {
	override = NULL;

	for (int i = 0; i < array_size(overrides); i++) {
		if (!strcmp(core_name, overrides[i].core_name)) {
			override = &overrides[i];
			break;
		}
	}
}
