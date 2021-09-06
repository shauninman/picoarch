#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "main.h"
#include "patch.h"
#include "util.h"

typedef int (*patch_func)(const uint8_t *in, size_t in_size,
                          const uint8_t *patch, size_t patch_size,
                          uint8_t *out, size_t *out_size);

static uint32_t crc32_table[256];

static void crc32_build_table(void) {
	uint32_t i, j;

	for (i = 0; i < array_size(crc32_table); i++) {
		crc32_table[i] = i;

		for (j = 8; j; j--) {
			crc32_table[i] = crc32_table[i] & 1
				? (crc32_table[i] >> 1) ^ 0xEDB88320
				: crc32_table[i] >> 1;
		}
	}
}

static uint32_t crc32(const uint8_t *buf, size_t len) {
	uint32_t hash = -1;

	if (crc32_table[1] == 0)
		crc32_build_table();

	while (len--) {
		int index = (uint8_t)(hash ^ *buf);
		hash = crc32_table[index] ^ (hash >> 8);
		buf++;
	}

	return ~hash;
}

enum bps_mode {
	BPS_MODE_SRC_READ,
	BPS_MODE_DST_READ,
	BPS_MODE_SRC_COPY,
	BPS_MODE_DST_COPY,
};

static uint64_t bps_decode(const uint8_t *patch, size_t *index) {
	uint64_t data = 0, shift = 1;

	while (1) {
		uint8_t x = patch[(*index)++];
		data += (x & 0x7f) * shift;
		if (x & 0x80) break;
		shift <<= 7;
		data += shift;
	}

	return data;
}

static int patch_bps(const uint8_t *in, size_t in_size,
              const uint8_t *patch, size_t patch_size,
              uint8_t *out, size_t *out_size) {
	size_t src_size, target_size, metadata_size;
	size_t written = 0;
	size_t src_rel_offset = 0, dst_rel_offset = 0;
	uint32_t source_hash = 0, patch_hash = 0, target_hash = 0;
	size_t i = 0;
	int j = 0;

	if (patch_size < 19) {
		return -1;
	}

	i = 4; /* skip BPS1 header */

	src_size = bps_decode(patch, &i);
	if (src_size != in_size) {
		PA_ERROR("Input size does not match expected size: %d != %d\n", in_size, src_size);
		return -1;
	}

	target_size = bps_decode(patch, &i);

	if (!out) {
		*out_size = target_size;
		return 0;
	} else if (*out_size != target_size) {
		PA_ERROR("Output size does not match expected size: %d != %d\n", in_size, target_size);
		return -1;
	}

	metadata_size = bps_decode(patch, &i);
	i += metadata_size;

	while(i < patch_size - 12) {
		size_t length = bps_decode(patch, &i);
		enum bps_mode mode = length & 3;
		int offset = 0;
		length = (length >> 2) + 1;

		switch(mode) {
		case BPS_MODE_SRC_READ:
			while (length--) {
				out[written] = in[written];
				written++;
			}
			break;
		case BPS_MODE_DST_READ:
			while (length--)
				out[written++] = patch[i++];
			break;
		case BPS_MODE_SRC_COPY:
		case BPS_MODE_DST_COPY:
			offset = bps_decode(patch, &i);
			offset = offset & 1 ? -(offset >> 1) : (offset >> 1);

			if (mode == BPS_MODE_SRC_COPY) {
				src_rel_offset += offset;
				while (length--)
					out[written++] = in[src_rel_offset++];
			} else {
				dst_rel_offset += offset;
				while (length--)
					out[written++] = out[dst_rel_offset++];
			}
			break;
		}
	}

	if (written != target_size) {
		PA_ERROR("Did not write expected number of bytes: %d != %d\n", written, target_size);
		return -1;
	}

	for (j = 0; j < 32; j += 8) {
		source_hash |= patch[i++] << j;
	}

	for (j = 0; j < 32; j += 8) {
		target_hash |= patch[i++] << j;
	}

	for (j = 0; j < 32; j += 8) {
		patch_hash |= patch[i++] << j;
	}

	if (source_hash != crc32(in, in_size)) {
		PA_ERROR("Input hash did not match: 0x%X != 0x%X\n", source_hash, crc32(in, in_size));
		return -1;
	}

	if (target_hash != crc32(out, *out_size)) {
		PA_ERROR("Output hash did not match: 0x%X != 0x%X\n", target_hash, crc32(out, *out_size));
		return -1;
	}

	if (patch_hash != crc32(patch, patch_size - 4)) {
		PA_ERROR("Patch hash did not match: 0x%X != 0x%X\n", patch_hash, crc32(patch, patch_size));
		return -1;
	}

	return 0;
}

static int patch_ips(const uint8_t *in, size_t in_size,
              const uint8_t *patch, size_t patch_size,
              uint8_t *out, size_t *out_size) {
	int i = 0;
	int ret = -1;
	uint32_t offset = 0;
	uint16_t len = 0;

	/* Needs at last PATCH and EOF */
	if (patch_size < 8)
		return -1;

	if (out == NULL && !strncmp((const char *)&patch[patch_size - 6], "EOF", 3)) {
		i = patch_size - 3;
		*out_size  = patch[i++] << 16;
		*out_size |= patch[i++] << 8;
		*out_size |= patch[i++];
		return 0;
	}

	i = 5; /* Skip PATCH header */

	while (i <= patch_size - 3) {
		offset  = patch[i++] << 16;
		offset |= patch[i++] << 8;
		offset |= patch[i++];
		
		if (offset == 0x454f46) {
			ret = 0;
			break;
		} else if (i <= patch_size - 2) {
			len  = patch[i++] << 8;
			len |= patch[i++];

			if (len) {
				if (i > patch_size - len)
					break;
				
				if (out) {
					while (len-- && offset < *out_size) {
						out[offset++] = patch[i++];
					}
				} else {
					i += len;
				}
			} else if (i <= patch_size - 3) { /* RLE */
				len  = patch[i++] << 8;
				len |= patch[i++];

				if (out) {
					while (len-- && offset < *out_size) {
						out[offset++] = patch[i];
					}
				}
				i++;
			} else {
				break;
			}

			if (!out && *out_size < offset + len)
				*out_size = offset + len;
		}
	}

	if (!out) {
		if (ret) {
			*out_size = 0;
		} else if (*out_size < in_size) {
			*out_size = in_size;
		}
	}

	return ret;
}

int patch(const void *in, size_t in_size,
          const uint8_t *patch, size_t patch_size,
          void **out, size_t *out_size) {
	int ret = -1;
	patch_func patch_apply;

	if (patch_size >= 8 && !strncmp((const char *)patch, "PATCH", sizeof("PATCH") - 1)) {
		patch_apply = patch_ips;
	} else if (patch_size >= 19 && !strncmp((const char *)patch, "BPS1", sizeof("BPS1") - 1)) {
		patch_apply = patch_bps;
	} else {
		PA_ERROR("Couldn't detect format of patch\n");
		goto finish;
	}

	if (patch_apply(in, in_size, patch, patch_size, NULL, out_size)) {
		PA_ERROR("Couldn't calculate output file size\n");
		goto finish;
	}

	if (*out_size == 0) {
		goto finish;
	}

	*out = (uint8_t *)calloc(*out_size, sizeof(uint8_t));
	if (!*out) {
		PA_ERROR("Couldn't allocate memory for patched output\n");
		goto finish;
	}

	memcpy(*out, in, MIN(in_size, *out_size));
	
	if (patch_apply(in, in_size, patch, patch_size, *out, out_size)) {
		PA_ERROR("Error patching output\n");
		goto finish;
	}
	ret = 0;

finish:
	if (ret) {
		free(*out);
		*out = NULL;
		*out_size = 0;
	}

	return ret;
}
