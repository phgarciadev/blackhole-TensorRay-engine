/**
 * @file sun.c
 * @brief Implementação do Sol
 */

#include "planet.h"
#include <math.h>

/* Visual: Plasma Solar simples */
static struct bhs_vec3 sun_surface_color(struct bhs_vec3 p)
{
	/* Emissão constante (branco-amarelado) no centro, darken nas bordas (limb darkening) */
	/* p é normalizado */
	
	/* Cor base: Amarelo-Branco Quente (~5800K blackbody approx) */
	struct bhs_vec3 base = { 1.0, 0.95, 0.8 };
	
	/* Turbulência simples baseada na posição (placeholder para noise) */
	/* Como não temos noise lib aqui, usamos um padrão estático matemático */
	double turbulence = sin(p.x * 20.0) * cos(p.y * 20.0) * sin(p.z * 20.0);
	
	struct bhs_vec3 color = {
		base.x + turbulence * 0.05,
		base.y + turbulence * 0.03,
		base.z
	};
	
	return color;
}

struct bhs_planet_desc bhs_sun_get_desc(void)
{
	struct bhs_planet_desc d = {0};
	
	d.name = "Sol";
	d.type = BHS_STAR_MAIN_SEQ;
	
	/* Essencial */
	d.mass = 1.9885e30;
	d.radius = 6.957e8;
	d.density = 1408.0;
	d.rotation_period = 25.05 * 24.0 * 3600.0; /* ~25 dias no equador */
	d.axis_tilt = 7.25 * (3.14159 / 180.0);
	d.gravity = 274.0;
	
	/* Orbital (Centro do sistema = 0) */
	d.semimajor_axis = 0.0;
	d.eccentricity = 0.0;
	d.orbital_period = 0.0;
	
	/* Atmosfera/Superfície */
	d.has_atmosphere = true; /* Fotosfera */
	d.mean_temperature = 5772.0;
	d.albedo = 0.0; /* Emissivo */
	
	/* Visual */
	d.base_color = (struct bhs_vec3){ 1.0, 0.9, 0.6 };
	d.get_surface_color = sun_surface_color;
	
	return d;
}

BHS_REGISTER_PLANET("Sol", bhs_sun_get_desc);
