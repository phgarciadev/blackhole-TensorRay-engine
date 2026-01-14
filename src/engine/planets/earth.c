/**
 * @file earth.c
 * @brief Implementação da Terra
 */

#include "planet.h"
#include <math.h>

static struct bhs_vec3 earth_surface_color(struct bhs_vec3 p)
{
	/* Simulação simplificada de continentes e oceanos baseada em noise (placeholder) */
	/* p normalizado */
	
	double noise = sin(p.x * 4.0) * cos(p.y * 4.0) * sin(p.z * 4.0);
	
	if (noise > 0.0) {
		/* Continente (Verde/Marrom) */
		return (struct bhs_vec3){ 0.2, 0.5, 0.1 };
	} else {
		/* Oceano (Azul) */
		return (struct bhs_vec3){ 0.0, 0.2, 0.7 };
	}
}

struct bhs_planet_desc bhs_earth_get_desc(void)
{
	struct bhs_planet_desc d = {0};
	
	d.name = "Terra";
	d.type = BHS_PLANET_TERRESTRIAL;
	
	/* Essencial */
	d.mass = 5.972e24;
	d.radius = 6.371e6;
	d.density = 5514.0;
	d.rotation_period = 23.9345 * 3600.0;
	d.axis_tilt = 23.44 * (3.14159 / 180.0);
	d.gravity = 9.807;
	
	/* Orbital */
	d.semimajor_axis = 1.496e11;
	d.eccentricity = 0.0167;
	d.orbital_period = 365.256 * 24 * 3600;
	
	/* Atmosfera */
	d.has_atmosphere = true;
	d.surface_pressure = 101325.0;
	d.mean_temperature = 288.0;
	d.albedo = 0.306;
	
	/* Visual */
	d.base_color = (struct bhs_vec3){ 0.0, 0.0, 1.0 }; /* Marble Blue */
	d.get_surface_color = earth_surface_color;
	
	return d;
}

BHS_REGISTER_PLANET("Terra", bhs_earth_get_desc);
