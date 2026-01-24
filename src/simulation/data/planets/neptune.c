/**
 * @file neptune.c
 * @brief Implementação de Netuno
 */

#include "../planet.h"
#include <math.h>

static struct bhs_vec3 neptune_surface_color(struct bhs_vec3 p)
{
	/* Azul profundo intenso */
	struct bhs_vec3 base = { 0.1, 0.2, 0.8 };
	
	/* Manchas escuras (tempestades) */
	double storm = sin(p.x * 2.0 + p.y * 3.0);
	if (storm > 0.8) return (struct bhs_vec3){ 0.05, 0.1, 0.5 };
	
	/* Nuvens brancas altas de metano */
	double cloud = sin(p.x * 10.0) * cos(p.y * 10.0);
	if (cloud > 0.95) return (struct bhs_vec3){ 0.9, 0.9, 1.0 };
	
	return base;
}

struct bhs_planet_desc bhs_neptune_get_desc(void)
{
	struct bhs_planet_desc d = {0};
	
	d.name = "Netuno";
	d.type = BHS_PLANET_ICE_GIANT;
	
	/* Essencial */
	d.mass = 1.02e26;
	d.radius = 2.4622e7;
	d.density = 1638.0;
	d.rotation_period = 16.11 * 3600.0;
	d.axis_tilt = 28.32 * (3.14159 / 180.0);
	d.gravity = 11.15;
	
	/* Orbital */
	d.semimajor_axis = 30.06896348 * 149597870700.0;
	d.eccentricity = 0.00858587;
	d.orbital_period = 164.8 * 365.25 * 24 * 3600;
	
	/* J2000 Elements */
	d.inclination = 1.76917;
	d.long_asc_node = 131.72169;
	d.long_perihelion = 44.97135;
	d.mean_longitude = 304.88003;
	
	/* Termodinâmico */
	d.has_atmosphere = true;
	d.surface_pressure = 1e9;
	d.mean_temperature = 72.0;
	d.albedo = 0.41;
	
	/* Visual */
	d.base_color = (struct bhs_vec3){ 0.1, 0.2, 0.8 };
	d.get_surface_color = neptune_surface_color;
	
	return d;
}

BHS_REGISTER_PLANET("Netuno", bhs_neptune_get_desc)
