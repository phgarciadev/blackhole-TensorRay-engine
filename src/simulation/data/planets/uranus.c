/**
 * @file uranus.c
 * @brief Implementação de Urano
 */

#include <math.h>
#include "../planet.h"

static struct bhs_vec3 uranus_surface_color(struct bhs_vec3 p)
{
	(void)p;
	/* Ciano pálido suave, quase sem features visíveis */
	return (struct bhs_vec3){ 0.5, 0.8, 0.9 };
}

struct bhs_planet_desc bhs_uranus_get_desc(void)
{
	struct bhs_planet_desc d = { 0 };

	d.name = "Urano";
	d.type = BHS_PLANET_ICE_GIANT;

	/* Essencial */
	d.mass = 8.68e25;
	d.radius = 2.5362e7;
	d.density = 1271.0;
	d.rotation_period = -17.24 * 3600.0;	 /* Retrógrado */
	d.axis_tilt = 97.77 * (3.14159 / 180.0); /* Deitado */
	d.gravity = 8.69;

	/* Orbital */
	d.semimajor_axis = 19.19126393 * 149597870700.0;
	d.eccentricity = 0.04716771;
	d.orbital_period = 84.0 * 365.25 * 24 * 3600;

	/* J2000 Elements */
	d.inclination = 0.76986;
	d.long_asc_node = 74.22988;
	d.long_perihelion = 170.96424;
	d.mean_longitude = 313.23218;

	/* Termodinâmico */
	d.has_atmosphere = true;
	d.surface_pressure = 1e9;
	d.mean_temperature = 59.0;
	d.albedo = 0.51;

	/* Visual */
	d.base_color = (struct bhs_vec3){ 0.5, 0.8, 0.9 };
	d.get_surface_color = uranus_surface_color;

	return d;
}

BHS_REGISTER_PLANET("Urano", bhs_uranus_get_desc)
