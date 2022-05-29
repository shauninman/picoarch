#ifndef PATCH_H
#define PATCH_H

int patch(const void *in, size_t in_size,
          const uint8_t *patch, size_t patch_size,
          void **out, size_t *out_size);
#endif
