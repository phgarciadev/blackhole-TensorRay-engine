/**
 * @file saturn.c
 * @brief Implementação de Saturno
 */

#include <math.h>
#include "../planet.h"

static struct bhs_vec3 saturn_surface_color(struct bhs_vec3 p)
{
	/* Amarelo pálido dourado, bandas suaves */
	struct bhs_vec3 base = { 0.85, 0.75, 0.5 };

	double band = sin(p.y * 15.0);

	return (struct bhs_vec3){ base.x + band * 0.03, base.y + band * 0.03,
				  base.z + band * 0.01 };
}

struct bhs_planet_desc bhs_saturn_get_desc(void)
{
	struct bhs_planet_desc d = { 0 };

	d.name = "Saturno";
	d.type = BHS_PLANET_GAS_GIANT;

	/* Essencial */
	d.mass = 5.68e26;
	d.radius = 5.8232e7;
	d.density = 687.0; /* Menor que a água! */
	d.rotation_period = 10.656 * 3600;
	d.axis_tilt = 0.4665; /* rad (26.73 deg) */
	d.gravity = 10.44;    /* m/s^2 */
	d.j2 = 16.29e-3;      /* Oblateness (very significant) */

	/* Orbital */
	d.semimajor_axis = 9.53707032 * 149597870700.0;
	d.eccentricity = 0.05415060;
	d.orbital_period = 29.45 * 365.25 * 24 * 3600;

	/* J2000 Elements */
	d.inclination = 2.48446;
	d.long_asc_node = 113.71504;
	d.long_perihelion = 92.43194;
	d.mean_longitude = 49.944;

	/* Termodinâmico */
	d.has_atmosphere = true;
	d.surface_pressure = 1e9;
	d.mean_temperature = 95.0;
	d.albedo = 0.47;

	/* Visual */
	d.base_color = (struct bhs_vec3){ 0.85, 0.75, 0.5 };
	d.get_surface_color = saturn_surface_color;

	/* TODO: Implementar anéis (sistema separado?) */

	return d;
}

BHS_REGISTER_PLANET("Saturno", bhs_saturn_get_desc)
