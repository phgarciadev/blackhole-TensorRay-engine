/**
 * @file image_gen.c
 * @brief Gerador Procedural de Texturas
 */

#include <math.h>
#include <stdlib.h>
#include "image_loader.h"

bhs_image_t bhs_image_gen_sphere(int size)
{
	bhs_image_t img = { 0 };
	img.width = size;
	img.height = size;
	img.channels = 4;
	img.data = malloc(size * size * 4);

	if (!img.data)
		return img;

	/* Lighting params */
	float light_x = -0.5f;
	float light_y = -0.5f;
	float light_z = 0.7071f; /* Normalize later if needed */
	float mag = sqrtf(light_x * light_x + light_y * light_y +
			  light_z * light_z);
	light_x /= mag;
	light_y /= mag;
	light_z /= mag;

	for (int y = 0; y < size; y++) {
		for (int x = 0; x < size; x++) {
			/* Normalized coords: -1 to 1 */
			float u = (float)x / (float)(size - 1) * 2.0f - 1.0f;
			float v = (float)y / (float)(size - 1) * 2.0f - 1.0f;

			float r2 = u * u + v * v;
			uint8_t *p = &img.data[(y * size + x) * 4];

			if (r2 > 1.0f) {
				p[0] = 0;
				p[1] = 0;
				p[2] = 0;
				p[3] = 0; /* Transparent */
			} else {
				float z = sqrtf(1.0f - r2);
				/* Normal = (u, v, z) */

				/* Diffuse: N dot L */
				float diff =
					u * light_x + v * light_y + z * light_z;
				if (diff < 0.0f)
					diff = 0.0f;

				/* Ambient */
				float ambient = 0.2f;

				/* Specular (Phong) */
				/* View vector = (0,0,1) */
				/* Reflect L around N: R = 2*(N.L)*N - L */
				/* R.V = R.z (since V=(0,0,1)) */
				// float Rx = 2.0f * diff * u - light_x;
				// float Ry = 2.0f * diff * v - light_y;
				float Rz = 2.0f * diff * z - light_z;

				float spec = 0.0f;
				if (Rz > 0.0f) {
					spec = powf(Rz, 20.0f) *
					       0.4f; /* Shininess */
				}

				float intensity = ambient + diff * 0.8f + spec;
				if (intensity > 1.0f)
					intensity = 1.0f;

				uint8_t val = (uint8_t)(intensity * 255.0f);

				p[0] = val; /* White sphere, tinted later by color */
				p[1] = val;
				p[2] = val;
				p[3] = 255; /* Opaque inside circle */

				/* Anti-aliasing soft edge */
				if (r2 > 0.95f) {
					float alpha = (1.0f - r2) / 0.05f;
					p[3] = (uint8_t)(alpha * 255.0f);
				}
			}
		}
	}

	return img;
}
