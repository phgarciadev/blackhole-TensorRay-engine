/**
 * @file mercury.c
 * @brief Implementação de Mercúrio
 */

#include "../planet.h"
#include <math.h>

static struct bhs_vec3 mercury_surface_color(struct bhs_vec3 p)
{
	/* Cinza escuro, crateras (simulado) */
	struct bhs_vec3 gray = { 0.4, 0.38, 0.35 };
	
	/* Simulação de crateras simples */
	double crater = sin(p.x * 50.0) * sin(p.y * 50.0);
	if (crater > 0.9) gray.x *= 0.8; /* Sombra */
	
	return gray;
}

struct bhs_planet_desc bhs_mercury_get_desc(void)
{
	struct bhs_planet_desc d = {0};
	
	d.name = "Mercurio";
	d.type = BHS_PLANET_TERRESTRIAL;
	
	/* Essencial */
	d.mass = 3.30e23;
	d.radius = 2.4397e6;
	d.density = 5427.0;
	/* Mercury spin is coupled 3:2 with orbit */
	d.rotation_period = 58.646 * 24 * 3600; /* 1407.6 hours */
	d.axis_tilt = 0.034 * (3.14159 / 180.0); /* Almost upright */
	d.gravity = 3.7;
	
	/* Orbital */
	d.semimajor_axis = 0.38709893 * 149597870700.0; /* ~0.387 AU */
	d.eccentricity = 0.20563069;
	d.orbital_period = 87.969 * 24 * 3600;

	/* J2000 Elements */
	d.inclination = 7.00487;
	d.long_asc_node = 48.33167;
	d.long_perihelion = 77.45645;
	d.mean_longitude = 252.25084;
	
	/* Termodinâmico */
	d.has_atmosphere = false;
	d.surface_pressure = 0.0;
	d.mean_temperature = 440.0; /* Média dia/noite */
	d.albedo = 0.119;
	
	/* Visual */
	d.base_color = (struct bhs_vec3){ 0.5, 0.5, 0.5 };
	d.get_surface_color = mercury_surface_color;
	
	return d;
}

BHS_REGISTER_PLANET("Mercurio", bhs_mercury_get_desc)
