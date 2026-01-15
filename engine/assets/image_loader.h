#ifndef BHS_LIB_LOADER_IMAGE_H
#define BHS_LIB_LOADER_IMAGE_H

#include <stdint.h>

/**
 * @struct bhs_image
 * @brief Raw image data container (RAM)
 */
typedef struct bhs_image {
	int width;
	int height;
	int channels;  /* 4 = RGBA */
	uint8_t *data; /* Raw pixels. Must be freed. */
} bhs_image_t;

/**
 * @brief Load an image from disk into RAM.
 * Ensures 4 channels (RGBA) for GPU compatibility.
 * Returns {0} on failure.
 */
bhs_image_t bhs_image_load(const char *path);

/**
 * @brief Free image memory.
 */
void bhs_image_free(bhs_image_t img);

/**
 * @brief Generate a 3D sphere impostor texture.
 * Returns RGBA image. User must free.
 */
bhs_image_t bhs_image_gen_sphere(int size);

#endif /* BHS_LIB_LOADER_IMAGE_H */
