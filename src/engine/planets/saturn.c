/**
 * @file saturn.c
 * @brief Implementação de Saturno
 */

#include "planet.h"
#include <math.h>

static struct bhs_vec3 saturn_surface_color(struct bhs_vec3 p)
{
	/* Amarelo pálido dourado, bandas suaves */
	struct bhs_vec3 base = { 0.85, 0.75, 0.5 };
	
	double band = sin(p.y * 15.0);
	
	return (struct bhs_vec3){
		base.x + band * 0.03,
		base.y + band * 0.03,
		base.z + band * 0.01
	};
}

struct bhs_planet_desc bhs_saturn_get_desc(void)
{
	struct bhs_planet_desc d = {0};
	
	d.name = "Saturno";
	d.type = BHS_PLANET_GAS_GIANT;
	
	/* Essencial */
	d.mass = 5.6834e26;
	d.radius = 5.8232e7;
	d.density = 687.0; /* Menor que a água! */
	d.rotation_period = 10.7 * 3600.0;
	d.axis_tilt = 26.73 * (3.14159 / 180.0);
	d.gravity = 10.44;
	
	/* Orbital */
	d.semimajor_axis = 1.4335e12;
	d.eccentricity = 0.0565;
	d.orbital_period = 29.45 * 365.25 * 24 * 3600;
	
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
