/**
 * @file haumea.c
 * @brief Implementação de Haumea
 */

#include "../planet.h"
#include <math.h>

static struct bhs_vec3 haumea_surface_color(struct bhs_vec3 p)
{
	/* Superfície de gelo cristalino muito brilhante */
	struct bhs_vec3 base = { 0.9, 0.9, 1.0 };
	
	/* 
	 * Mancha vermelha escura (Dark Red Spot) 
	 * Latitude ~ -20°, Longitude ~ 200° 
	 * No espaço local (esférico), simulamos uma área específica.
	 */
	double spot_dist = sqrt(pow(p.x - 0.5, 2) + pow(p.y + 0.3, 2) + pow(p.z - 0.2, 2));
	if (spot_dist < 0.25) {
		return (struct bhs_vec3){ 0.5, 0.2, 0.1 }; /* Vermelho escuro/marrom */
	}
	
	/* Ruído sutil de gelo */
	double noise = sin(p.x * 50.0) * cos(p.z * 50.0) * 0.05;
	base.x += noise;
	base.y += noise;
	base.z += noise;
	
	return base;
}

struct bhs_planet_desc bhs_haumea_get_desc(void)
{
	struct bhs_planet_desc d = {0};
	
	d.name = "Haumea";
	d.type = BHS_PLANET_DWARF;
	
	/* Físico (Haumea é elipsoidal, usamos raio médio volumétrico) */
	d.mass = 4.006e21;
	d.radius = 8.16e5;
	d.density = 2018.0;
	d.rotation_period = 3.91 * 3600.0; /* 14076s */
	d.axis_tilt = 126.0 * (3.14159 / 180.0);
	d.gravity = 0.401;
	
	/* Orbital J2000.0 */
	d.semimajor_axis = 43.132 * 149597870700.0;
	d.eccentricity = 0.19126;
	d.orbital_period = 285.0 * 365.25 * 24 * 3600;
	
	d.inclination = 28.21;
	d.long_asc_node = 122.10;
	d.long_perihelion = 1.30; /* Omega + omega = 122.10 + 239.20 = 361.30 mod 360 */
	d.mean_longitude = 199.70; /* varpi + M = 1.30 + 198.40 = 199.70 */
	
	/* Termodinâmico */
	d.has_atmosphere = false;
	d.surface_pressure = 0.0;
	d.mean_temperature = 50.0;
	d.albedo = 0.75;
	
	/* Visual */
	d.base_color = (struct bhs_vec3){ 0.9, 0.9, 1.0 };
	d.get_surface_color = haumea_surface_color;
	
	return d;
}

BHS_REGISTER_PLANET("Haumea", bhs_haumea_get_desc)
