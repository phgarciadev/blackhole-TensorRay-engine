/**
 * @file body_factory.c
 * @brief Fábrica de Corpos Celestes (Updated for Kernel Style)
 */

#include "body.h"
#include <string.h>

struct bhs_body bhs_body_create_planet_simple(struct bhs_vec3 pos, double mass,
					      double radius,
					      struct bhs_vec3 color)
{
	struct bhs_body b = { 0 };
	
	/* Universal State */
	b.state.pos = pos;
	b.state.mass = (mass > 0.0) ? mass : 0.01; /* Earth-like relative to BH */
	b.state.radius = radius;
	b.state.acc = (struct bhs_vec3){ 0, 0, 0 };
	b.state.vel = (struct bhs_vec3){ 0, 0, 0 };
	b.state.rot_axis = (struct bhs_vec3){ 0, 1, 0 };
	b.state.rot_speed = 7.27e-5; /* Earth rotation rad/s */
	b.state.moment_inertia = 0.4 * b.state.mass * radius * radius; /* Sphere */
	b.state.shape = BHS_SHAPE_SPHERE;

	/* Type Specifics */
	b.type = BHS_BODY_PLANET;
	b.prop.planet = (struct bhs_planet_data){
		.physical_state = BHS_STATE_SOLID,
		.density = 5514.0,	   /* Earth kg/m3 */
		.surface_pressure = 1.0,   /* Atm */
		.atmosphere_mass = 5.1e18, /* kg */
		.composition = "N2 78%, O2 21%",
		.temperature = 288.0, /* 15 C */
		.albedo = 0.306,
		.axis_tilt = 0.409, /* 23.4 deg */
		.has_atmosphere = true,
		.has_magnetic_field = true
	};

	/* Visuals */
	b.color = color;
	return b;
}

struct bhs_body bhs_body_create_star_simple(struct bhs_vec3 pos, double mass,
					    double radius,
					    struct bhs_vec3 color)
{
	struct bhs_body b = { 0 };

	/* Universal State */
	b.state.pos = pos;
	b.state.mass = (mass > 0.0) ? mass : 2.0; /* Sol-like */
	b.state.radius = radius;
	b.state.rot_axis = (struct bhs_vec3){ 0, 1, 0 };
	b.state.rot_speed = 2.9e-6; /* Sun rotation rad/s */
	b.state.moment_inertia = 0.07 * b.state.mass * radius * radius; /* Central condensed */
	b.state.shape = BHS_SHAPE_SPHERE;

	/* Type Specifics */
	b.type = BHS_BODY_STAR;
	b.prop.star = (struct bhs_star_data){
		.luminosity = 3.828e26, /* Watts */
		.temp_effective = 5772.0, /* Kelvin */
		.age = 4.6e9,		  /* Years */
		.stage = BHS_STAR_MAIN_SEQUENCE,
		.metallicity = 0.0122,
		.spectral_type = "G2V"
	};

	/* Visuals */
	b.color = color;
	return b;
}

struct bhs_body bhs_body_create_blackhole_simple(struct bhs_vec3 pos,
						 double mass, double radius)
{
	struct bhs_body b = { 0 };

	/* Universal State */
	b.state.pos = pos;
	b.state.mass = (mass > 0.0) ? mass : 10.0; /* 5x Sol mass BH */
	b.state.radius = radius; /* Visual Horizon */
	b.state.rot_axis = (struct bhs_vec3){ 0, 1, 0 };
	b.state.shape = BHS_SHAPE_SPHERE;
	
	/* Type Specifics */
	b.type = BHS_BODY_BLACKHOLE;
	b.prop.bh = (struct bhs_blackhole_data){
		.spin_factor = 0.9,
		.event_horizon_r = 2.0 * b.state.mass,
		.ergososphere_r = 2.0 * b.state.mass, /* Simplified (static limit at equator) */
		.accretion_disk_mass = 0.01 * b.state.mass
	};

	/* Visuals */
	b.color = (struct bhs_vec3){ 0.0, 0.0, 0.0 };
	return b;
}
