/**
 * @file eris.c
 * @brief Implementação de Éris
 */

#include "../planet.h"
#include <math.h>

static struct bhs_vec3 eris_surface_color(struct bhs_vec3 p)
{
	/* 
	 * Superfície extremamente brilhante (alto albedo, ~0.96)
	 * Gelo de nitrogênio e metano pálido.
	 */
	struct bhs_vec3 base = { 0.95, 0.95, 0.98 };
	
	/* Ruído sutil de granulação de geada */
	double frost = sin(p.x * 60.0) * cos(p.y * 60.0) * cos(p.z * 60.0);
	double variation = frost * 0.02;
	
	base.x += variation;
	base.y += variation;
	base.z += variation;
	
	/* Manchas pálidas de terreno mais antigo ou tolinas diluídas */
	double patches = sin(p.x * 5.0 + p.z * 3.0);
	if (patches > 0.9) {
		base.x *= 0.98;
		base.y *= 0.96;
		base.z *= 0.94;
	}
	
	return base;
}

struct bhs_planet_desc bhs_eris_get_desc(void)
{
	struct bhs_planet_desc d = {0};
	
	d.name = "Eris";
	d.type = BHS_PLANET_DWARF;
	
	/* Físico (JPL) */
	d.mass = 1.66e22;
	d.radius = 1.163e6;
	d.density = 2520.0;
	d.rotation_period = 25.9 * 3600.0;
	d.axis_tilt = 0.0; /* Assumindo vertical por falta de dados axiais precisos */
	d.gravity = 0.82;
	
	/* Orbital J2000.0 (JD 2451545.0) */
	d.semimajor_axis = 67.6681 * 149597870700.0;
	d.eccentricity = 0.44177;
	d.orbital_period = 557.0 * 365.25 * 24 * 3600;
	
	d.inclination = 44.187;
	d.long_asc_node = 35.869;
	d.long_perihelion = 187.304; /* Omega + omega = 35.869 + 151.435 = 187.304 */
	d.mean_longitude = 25.19;
	
	/* Termodinâmico */
	d.has_atmosphere = false; /* Atmosfera colapsada na superfície (geada) */
	d.surface_pressure = 0.0;
	d.mean_temperature = 30.0;
	d.albedo = 0.96;
	
	/* Visual */
	d.base_color = (struct bhs_vec3){ 0.95, 0.95, 0.98 };
	d.get_surface_color = eris_surface_color;
	
	return d;
}

BHS_REGISTER_PLANET("Eris", bhs_eris_get_desc)
