/**
 * @file venus.c
 * @brief Implementação de Vênus
 */

#include <math.h>
#include "../planet.h"

static struct bhs_vec3 venus_surface_color(struct bhs_vec3 p)
{
	/* Vênus é coberto por nuvens espessas de ácido sulfúrico */
	/* Cor: Amarelo pálido / creme */
	struct bhs_vec3 base = { 0.9, 0.85, 0.7 };

	/* Bandas atmosféricas sutis */
	double band = sin(p.y * 10.0 + sin(p.x * 5.0));

	return (struct bhs_vec3){ base.x + band * 0.05, base.y + band * 0.04,
				  base.z + band * 0.02 };
}

struct bhs_planet_desc bhs_venus_get_desc(void)
{
	struct bhs_planet_desc d = { 0 };

	d.name = "Venus";
	d.type = BHS_PLANET_TERRESTRIAL;

	/* Essencial */
	d.mass = 4.87e24;
	d.radius = 6.0518e6;
	d.density = 5243.0;
	/* Venus rotates backwards (Retrograde) -243 days */
	d.rotation_period = -243.025 * 24 * 3600;
	d.axis_tilt = 177.36 * (3.14159 / 180.0); /* Upside down essentially */
	d.gravity = 8.87;

	/* Orbital */
	d.semimajor_axis = 0.72333199 * 149597870700.0;
	d.eccentricity = 0.00677323;
	d.orbital_period = 224.7 * 24 * 3600;

	/* J2000 Elements */
	d.inclination = 3.39471;
	d.long_asc_node = 76.68069;
	d.long_perihelion = 131.53298;
	d.mean_longitude = 181.97973;

	/* Atmosfera */
	d.has_atmosphere = true;
	d.surface_pressure = 9.2e6;
	d.mean_temperature = 737.0; /* Efeito estufa brutal */
	d.albedo = 0.75;

	/* Visual */
	d.base_color = (struct bhs_vec3){ 0.9, 0.85, 0.7 };
	d.get_surface_color = venus_surface_color;

	return d;
}

BHS_REGISTER_PLANET("Venus", bhs_venus_get_desc)
