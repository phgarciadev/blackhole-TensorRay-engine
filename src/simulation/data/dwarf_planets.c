/**
 * @file dwarf_planets.c
 * @brief Implementação de Planetas Anões (Plutão e Ceres)
 */

#include "planet.h"
#include <math.h>

/* --- PLUTÃO --- */

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
	
	d.mass = 1.303e22;
	d.radius = 1.1883e6;
	d.density = 1854.0;
	d.rotation_period = -6.387 * 24 * 3600;
	d.axis_tilt = 122.53 * (3.14159 / 180.0);
	d.gravity = 0.62;
	
	d.semimajor_axis = 5.906e12;
	d.eccentricity = 0.2488;
	d.orbital_period = 248.0 * 365.25 * 24 * 3600;
	
	d.has_atmosphere = true; /* Tênue, sazonal */
	d.surface_pressure = 1.0; /* Pa, varia muito */
	d.mean_temperature = 44.0;
	d.albedo = 0.5; // Variável
	
	d.base_color = (struct bhs_vec3){ 0.8, 0.7, 0.6 };
	d.get_surface_color = pluto_surface_color;
	
	return d;
}

/* --- CERES --- */

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
	
	d.mass = 9.393e20;
	d.radius = 4.73e5;
	d.density = 2161.0;
	d.rotation_period = 9.074 * 3600;
	d.axis_tilt = 0.05; /* ~3 graus */
	d.gravity = 0.27;
	
	d.semimajor_axis = 4.14e11; /* 2.77 AU */
	d.eccentricity = 0.076;
	d.orbital_period = 4.6 * 365.25 * 24 * 3600;
	
	d.has_atmosphere = false; /* Vapor de água detectado mas desprezível */
	d.surface_pressure = 0.0;
	d.mean_temperature = 168.0;
	d.albedo = 0.09;
	
	d.base_color = (struct bhs_vec3){ 0.35, 0.35, 0.35 };
	d.get_surface_color = ceres_surface_color;
	
	return d;
}

BHS_REGISTER_PLANET("Plutao", bhs_pluto_get_desc)
BHS_REGISTER_PLANET("Ceres", bhs_ceres_get_desc)
