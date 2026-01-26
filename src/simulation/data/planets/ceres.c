/**
 * @file ceres.c
 * @brief Implementação de Ceres
 */

#include "../planet.h"
#include <math.h>

static struct bhs_vec3 ceres_surface_color(struct bhs_vec3 p)
{
	/* Cinza escuro rochoso */
	struct bhs_vec3 base = { 0.35, 0.35, 0.35 };
	
	/* Pontos brilhantes (Occator crater spots) */
	/* Simplificado: Alguns pontos aleatórios de brilho intenso */
	double spots = sin(p.x * 30.0) * cos(p.y * 30.0);
	if (spots > 0.98) return (struct bhs_vec3){ 0.9, 0.9, 1.0 }; /* Salmoura */
	
	return base;
}

struct bhs_planet_desc bhs_ceres_get_desc(void)
{
	struct bhs_planet_desc d = {0};
	
	d.name = "Ceres";
	d.type = BHS_PLANET_DWARF;
	
	d.mass = 9.4e20;
	d.radius = 4.73e5;
	d.density = 2161.0;
	d.rotation_period = 9.074 * 3600;
	d.axis_tilt = 0.05; /* ~3 graus */
	d.gravity = 0.27;
	
	/* Orbital */
	d.semimajor_axis = 2.7663 * 149597870700.0;
	d.eccentricity = 0.0758;
	d.orbital_period = 4.6 * 365.25 * 24 * 3600;

	/* J2000 Elements */
	d.inclination = 10.593;
	d.long_asc_node = 80.31;
	d.long_perihelion = 153.9; /* varpi = Omega + omega = 80.31 + 73.59 */
	d.mean_longitude = 288.7;
	
	d.has_atmosphere = false; /* Vapor de água detectado mas desprezível */
	d.surface_pressure = 0.0;
	d.mean_temperature = 168.0;
	d.albedo = 0.09;
	
	d.base_color = (struct bhs_vec3){ 0.35, 0.35, 0.35 };
	d.get_surface_color = ceres_surface_color;
	
	return d;
}

BHS_REGISTER_PLANET("Ceres", bhs_ceres_get_desc)
