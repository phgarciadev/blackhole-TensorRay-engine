/**
 * @file jupiter.c
 * @brief Implementação de Júpiter
 */

#include "../planet.h"
#include <math.h>

static struct bhs_vec3 jupiter_surface_color(struct bhs_vec3 p)
{
	/* Bandas complexas */
	struct bhs_vec3 bands[] = {
		{0.6, 0.5, 0.4},  /* Marrom claro */
		{0.7, 0.6, 0.5},  /* Creme */
		{0.5, 0.3, 0.2},  /* Marrom escuro */
	};
	
	/* Turbulência de bandas */
	double y_dist = p.y * 10.0 + sin(p.x * 3.0) * 0.5;
	int band_idx = (int)fabs(y_dist) % 3;
	
	/* Grande Mancha Vermelha (simplificada) */
	double spot_dist = sqrt(pow(p.x - 0.5, 2) + pow(p.y + 0.3, 2));
	if (spot_dist < 0.2) {
		return (struct bhs_vec3){ 0.7, 0.2, 0.1 }; /* Vermelho tijolo */
	}
	
	return bands[band_idx];
}

struct bhs_planet_desc bhs_jupiter_get_desc(void)
{
	struct bhs_planet_desc d = {0};
	
	d.name = "Jupiter";
	d.type = BHS_PLANET_GAS_GIANT;
	
	/* Essencial */
	d.mass = 1.90e27;
	d.radius = 6.9911e7;
	d.density = 1326.0;
	d.rotation_period = 9.925 * 3600.0; /* ~10h */
	d.axis_tilt = 3.13 * (3.14159 / 180.0);
	d.gravity = 24.79;
	
	/* Orbital */
	d.semimajor_axis = 7.7857e11;
	d.eccentricity = 0.0489;
	d.orbital_period = 11.86 * 365.25 * 24 * 3600;
	
	/* Solitário Termodinâmico */
	d.has_atmosphere = true;
	d.surface_pressure = 1e9; /* Indefinido (transição fluido) */
	d.mean_temperature = 124.0; /* Efetiva */
	d.albedo = 0.503;
	
	/* Visual */
	d.base_color = (struct bhs_vec3){ 0.7, 0.6, 0.5 };
	d.get_surface_color = jupiter_surface_color;
	
	return d;
}

BHS_REGISTER_PLANET("Jupiter", bhs_jupiter_get_desc)
