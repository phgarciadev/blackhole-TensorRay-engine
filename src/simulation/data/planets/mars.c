/**
 * @file mars.c
 * @brief Implementação de Marte
 */

#include "../planet.h"
#include <math.h>

static struct bhs_vec3 mars_surface_color(struct bhs_vec3 p)
{
	/* Vermelho ferrugem */
	struct bhs_vec3 base = { 0.6, 0.2, 0.1 };
	
	/* Calotas polares */
	if (fabs(p.z) > 0.9) {
		return (struct bhs_vec3){ 0.9, 0.9, 0.95 }; /* Gelo de CO2/Agua */
	}
	
	return base;
}

struct bhs_planet_desc bhs_mars_get_desc(void)
{
	struct bhs_planet_desc d = {0};
	
	d.name = "Marte";
	d.type = BHS_PLANET_TERRESTRIAL;
	
	/* Essencial */
	d.mass = 6.42e23;
	d.radius = 3.3895e6;
	d.density = 3933.0;
	/* Mars Sol: 24h 37m 22s */
	d.rotation_period = 24.6229 * 3600.0;
	d.axis_tilt = 25.19 * (3.14159 / 180.0);
	d.gravity = 3.72;
	
	/* Orbital */
	d.semimajor_axis = 1.52366231 * 149597870700.0;
	d.eccentricity = 0.09341233;
	d.orbital_period = 686.98 * 24 * 3600;
	
	/* J2000 Elements */
	d.inclination = 1.85061;
	d.long_asc_node = 49.57854;
	d.long_perihelion = 336.04084;
	d.mean_longitude = 355.45332;
	
	/* Atmosfera */
	d.has_atmosphere = true; /* Fina */
	d.surface_pressure = 610.0;
	d.mean_temperature = 210.0;
	d.albedo = 0.25;
	
	/* Visual */
	d.base_color = (struct bhs_vec3){ 0.6, 0.2, 0.1 };
	d.get_surface_color = mars_surface_color;
	
	return d;
}

BHS_REGISTER_PLANET("Marte", bhs_mars_get_desc)
