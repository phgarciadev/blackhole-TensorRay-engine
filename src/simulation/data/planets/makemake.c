/**
 * @file makemake.c
 * @brief Implementação de Makemake
 */

#include "../planet.h"
#include <math.h>

static struct bhs_vec3 makemake_surface_color(struct bhs_vec3 p)
{
	/* Superfície avermelhada (tolinas) */
	struct bhs_vec3 base = { 0.6, 0.3, 0.2 };
	
	/* Manchas de gelo de metano branco-creme */
	double methane_ice = sin(p.x * 25.0) * cos(p.y * 15.0) * sin(p.z * 10.0);
	if (methane_ice > 0.85) {
		return (struct bhs_vec3){ 0.9, 0.85, 0.8 };
	}
	
	/* Variações de terreno */
	double var = cos(p.x * 40.0 + p.z * 30.0) * 0.05;
	base.x += var;
	base.y += var;
	
	return base;
}

struct bhs_planet_desc bhs_makemake_get_desc(void)
{
	struct bhs_planet_desc d = {0};
	
	d.name = "Makemake";
	d.type = BHS_PLANET_DWARF;
	
	/* Físico (Estimado) */
	d.mass = 3.1e21;
	d.radius = 7.15e5;
	d.density = 1700.0;
	d.rotation_period = 22.83 * 3600.0;
	d.axis_tilt = 0.0; /* Assumindo vertical por falta de dados exatos */
	d.gravity = 0.404;
	
	/* Orbital J2000.0 */
	d.semimajor_axis = 45.715 * 149597870700.0;
	d.eccentricity = 0.15586;
	d.orbital_period = 309.0 * 365.25 * 24 * 3600;
	
	d.inclination = 28.963;
	d.long_asc_node = 79.619;
	d.long_perihelion = 14.454; /* 79.619 + 294.835 = 374.454 mod 360 */
	d.mean_longitude = 156.41;
	
	/* Termodinâmico */
	d.has_atmosphere = false;
	d.surface_pressure = 0.0;
	d.mean_temperature = 30.0;
	d.albedo = 0.77;
	
	/* Visual */
	d.base_color = (struct bhs_vec3){ 0.6, 0.3, 0.2 };
	d.get_surface_color = makemake_surface_color;
	
	return d;
}

BHS_REGISTER_PLANET("Makemake", bhs_makemake_get_desc)
