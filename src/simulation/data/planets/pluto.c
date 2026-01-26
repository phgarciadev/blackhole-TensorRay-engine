/**
 * @file pluto.c
 * @brief Implementação de Plutão
 */

#include "../planet.h"
#include <math.h>

static struct bhs_vec3 pluto_surface_color(struct bhs_vec3 p)
{
	/* Creme acastanhado com "Coração" (Tombaugh Regio) */
	struct bhs_vec3 base = { 0.8, 0.7, 0.6 };
	
	/* Simulação rudimentar do coração */
	double heart_shape = pow(p.x - 0.2, 2) + pow(p.y * 1.5, 2);
	if (heart_shape < 0.1) {
		return (struct bhs_vec3){ 0.95, 0.9, 0.85 }; /* Branco gelo nitrogênio */
	}
	
	/* Manchas escuras (Cthulhu Macula) */
	if (p.x < -0.2 && fabs(p.y) < 0.3) {
		return (struct bhs_vec3){ 0.3, 0.2, 0.1 }; /* Tolin vermelhão escuro */
	}
	
	return base;
}

struct bhs_planet_desc bhs_pluto_get_desc(void)
{
	struct bhs_planet_desc d = {0};
	
	d.name = "Plutao";
	d.type = BHS_PLANET_DWARF;
	
	d.mass = 1.31e22;
	d.radius = 1.188e6;
	d.density = 1854.0;
	d.rotation_period = -6.387 * 24 * 3600;
	d.axis_tilt = 122.53 * (3.14159 / 180.0);
	d.gravity = 0.62;
	
	/* Orbital */
	d.semimajor_axis = 39.48168677 * 149597870700.0;
	d.eccentricity = 0.24880766;
	d.orbital_period = 248.0 * 365.25 * 24 * 3600;
	
	/* J2000 Elements */
	d.inclination = 17.14175;
	d.long_asc_node = 110.30347;
	d.long_perihelion = 224.06676;
	d.mean_longitude = 238.92881;
	
	d.has_atmosphere = true; /* Tênue, sazonal */
	d.surface_pressure = 1.0; /* Pa, varia muito */
	d.mean_temperature = 44.0;
	d.albedo = 0.5; // Variável
	
	d.base_color = (struct bhs_vec3){ 0.8, 0.7, 0.6 };
	d.get_surface_color = pluto_surface_color;
	
	return d;
}

BHS_REGISTER_PLANET("Plutao", bhs_pluto_get_desc)
