/**
 * @file sun.c
 * @brief Implementação do Sol
 */

#include "src/simulation/data/planet.h"
#include "src/simulation/data/sun.h"
#include <math.h>

/* Visual: Plasma Solar simples */
static struct bhs_vec3 sun_surface_color(struct bhs_vec3 p)
{
	/* Emissão constante (branco-amarelado) no centro, darken nas bordas (limb darkening) */
	/* p é normalizado */
	
	/* Cor base: Amarelo-Branco Quente (~5800K blackbody approx) */
	struct bhs_vec3 base = { 1.0, 0.95, 0.8 };
	
	/* Turbulência simples baseada na posição (placeholder para noise) */
	double turbulence = sin(p.x * 20.0) * cos(p.y * 20.0) * sin(p.z * 20.0);
	
	struct bhs_vec3 color = {
		base.x + turbulence * 0.05,
		base.y + turbulence * 0.03,
		base.z
	};
	
	return color;
}

/* 
 * Lógica dedicada para o Sol 
 * (Fonte da verdade)
 */
static const struct bhs_sun_desc THE_SUN = {
    .name = "Sol",
    .mass = 1.989e30,
    .radius = 6.9634e8,
    .temperature = 5772.0,
    .luminosity = 3.828e26,
    .age = 4.6e9,
    .metallicity = 0.0, /* Referência */
    .spectral_type = "G2V",
    .stage = BHS_SUN_MAIN_SEQUENCE,
    .rotation_period = 25.05 * 24.0 * 3600.0,
    .axis_tilt = 0.126, /* ~7.25 deg */
    .base_color = { 1.0, 0.9, 0.6 },
    .get_surface_color = sun_surface_color
};

/* Adapter para UI/Registry Legacy */
struct bhs_planet_desc bhs_sun_get_desc(void)
{
	struct bhs_planet_desc d = {0};
    const struct bhs_sun_desc *s = &THE_SUN;
	
	d.name = s->name;
	d.type = BHS_STAR_MAIN_SEQ;
	
	d.mass = s->mass;
	d.radius = s->radius;
	d.density = s->mass / (4.0/3.0 * 3.14159 * pow(s->radius, 3)); /* Derivada */
	d.rotation_period = s->rotation_period;
	d.axis_tilt = s->axis_tilt;
	d.gravity = (6.674e-11 * s->mass) / (s->radius * s->radius);
	
    /* Estrelas não orbitam planetas (neste contexto simples) */
	d.semimajor_axis = 0.0;
	d.eccentricity = 0.0;
	d.orbital_period = 0.0;
	
	d.has_atmosphere = true; /* Fotosfera */
	d.mean_temperature = s->temperature;
	d.albedo = 0.0; 
	
	d.base_color = s->base_color;
	d.get_surface_color = s->get_surface_color;
	
	return d;
}

BHS_REGISTER_PLANET("Sol", bhs_sun_get_desc)
