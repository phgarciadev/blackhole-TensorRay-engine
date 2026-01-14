/**
 * @file presets.c
 * @brief Implementação dos Corpos Celestes Pré-Definidos
 *
 * "Criar um Sol é fácil. Manter os planetas em órbita é a parte difícil."
 */

#include "engine/presets/presets.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/*
 * G_SIM: Constante gravitacional normalizada para nossa escala.
 *
 * Na escala real: G = 6.674e-11 m³/(kg·s²)
 * Na nossa escala (1u = 1e7m), precisamos ajustar G para que
 * a velocidade orbital v = sqrt(G*M/r) produza órbitas corretas.
 *
 * Usamos G = 1.0 (unidades naturais) e ajustamos as massas
 * para que a física funcione na escala visual.
 */
#define G_SIM 1.0

/*
 * MASS_SCALE: Fator para converter massa real em massa simulada.
 * Escolhido para que órbitas sejam estáveis com G_SIM = 1.
 */
#define MASS_SCALE (1.0 / 1e29)

/* ============================================================================
 * FUNÇÕES AUXILIARES
 * ============================================================================
 */

double bhs_preset_orbital_velocity(double central_mass, double orbital_radius)
{
	if (orbital_radius <= 0)
		return 0;

	/* v = sqrt(G * M / r) */
	return sqrt(G_SIM * central_mass / orbital_radius);
}

/* ============================================================================
 * FACTORIES
 * ============================================================================
 */

struct bhs_body bhs_preset_sun(struct bhs_vec3 pos)
{
	struct bhs_body sun = {0};

	/* Estado cinemático */
	sun.state.pos = pos;
	sun.state.vel = (struct bhs_vec3){0, 0, 0};
	sun.state.acc = (struct bhs_vec3){0, 0, 0};
	sun.state.mass = BHS_MASS_SUN * MASS_SCALE;  /* ~19.88 */
	sun.state.radius = 3.0;  /* Visualmente grande mas não absurdo */
	sun.state.rot_axis = (struct bhs_vec3){0, 1, 0};
	sun.state.rot_speed = 2.0 * M_PI / (25.38 * 86400);  /* rad/s */
	sun.state.shape = BHS_SHAPE_SPHERE;

	/* Metadados */
	sun.type = BHS_BODY_STAR;
	sun.color = (struct bhs_vec3){1.0, 0.9, 0.3};  /* Amarelo solar */
	sun.is_alive = true;
	sun.is_fixed = true;  /* Sol fica no centro */

	/* Dados específicos da estrela */
	sun.prop.star.luminosity = 3.828e26;     /* W */
	sun.prop.star.temp_effective = 5772.0;   /* K */
	sun.prop.star.age = 4.6e9;               /* anos */
	sun.prop.star.stage = BHS_STAR_MAIN_SEQUENCE;
	sun.prop.star.metallicity = 0.0122;      /* Z solar */
	strncpy(sun.prop.star.spectral_type, "G2V", sizeof(sun.prop.star.spectral_type));

	printf("[PRESET] Sol criado: M=%.2f, R=%.2f, T=%.0fK, fixo=sim\n",
	       sun.state.mass, sun.state.radius, sun.prop.star.temp_effective);

	return sun;
}

struct bhs_body bhs_preset_earth(struct bhs_vec3 sun_pos)
{
	struct bhs_body earth = {0};

	/* Posição: À direita do Sol */
	double orbit_r = BHS_SIM_ORBIT_EARTH;
	earth.state.pos = (struct bhs_vec3){
		.x = sun_pos.x + orbit_r,
		.y = 0,
		.z = sun_pos.z
	};

	/* Velocidade orbital perpendicular (anti-horário visto de cima) */
	double sun_mass = BHS_MASS_SUN * MASS_SCALE;
	double v_orb = bhs_preset_orbital_velocity(sun_mass, orbit_r);
	earth.state.vel = (struct bhs_vec3){0, 0, v_orb};

	earth.state.acc = (struct bhs_vec3){0, 0, 0};
	earth.state.mass = BHS_MASS_EARTH * MASS_SCALE;  /* ~0.0000597 */
	earth.state.radius = 0.8;  /* Visualmente proporcional */
	earth.state.rot_axis = (struct bhs_vec3){
		sin(23.44 * M_PI / 180.0), 
		cos(23.44 * M_PI / 180.0), 
		0
	};
	earth.state.rot_speed = 2.0 * M_PI / 86164.0;  /* rad/s (dia sideral) */
	earth.state.shape = BHS_SHAPE_SPHERE;

	/* Metadados */
	earth.type = BHS_BODY_PLANET;
	earth.color = (struct bhs_vec3){0.2, 0.4, 0.8};  /* Azul terrestre */
	earth.is_alive = true;
	earth.is_fixed = false;

	/* Dados específicos do planeta */
	earth.prop.planet.density = 5514.0;          /* kg/m³ */
	earth.prop.planet.axis_tilt = 23.44 * M_PI / 180.0;  /* rad */
	earth.prop.planet.albedo = 0.306;
	earth.prop.planet.has_atmosphere = true;
	earth.prop.planet.surface_pressure = 101325.0;  /* Pa */
	earth.prop.planet.atmosphere_mass = 5.15e18;    /* kg */
	strncpy(earth.prop.planet.composition, "N2 78%, O2 21%", 
		sizeof(earth.prop.planet.composition));
	earth.prop.planet.temperature = 288.0;  /* K */
	earth.prop.planet.physical_state = BHS_STATE_SOLID;
	earth.prop.planet.has_magnetic_field = true;

	printf("[PRESET] Terra criada: M=%.6f, R=%.2f, órbita=%.1f, v=%.3f\n",
	       earth.state.mass, earth.state.radius, orbit_r, v_orb);

	return earth;
}

struct bhs_body bhs_preset_moon(struct bhs_vec3 earth_pos,
				struct bhs_vec3 earth_vel)
{
	struct bhs_body moon = {0};

	/* Posição: À direita da Terra */
	double orbit_r = BHS_SIM_ORBIT_MOON;
	moon.state.pos = (struct bhs_vec3){
		.x = earth_pos.x + orbit_r,
		.y = earth_pos.y,
		.z = earth_pos.z
	};

	/* Velocidade: orbital ao redor da Terra + velocidade da Terra */
	double earth_mass = BHS_MASS_EARTH * MASS_SCALE;
	double v_orb = bhs_preset_orbital_velocity(earth_mass, orbit_r);

	moon.state.vel = (struct bhs_vec3){
		.x = earth_vel.x,
		.y = earth_vel.y,
		.z = earth_vel.z + v_orb  /* Soma velocidade orbital lunar */
	};

	moon.state.acc = (struct bhs_vec3){0, 0, 0};
	moon.state.mass = BHS_MASS_MOON * MASS_SCALE;  /* ~7.34e-7 */
	moon.state.radius = 0.3;  /* Visualmente proporcional */
	/* Rotação travada (síncrona) */
	moon.state.rot_axis = (struct bhs_vec3){0, 1, 0};
	moon.state.rot_speed = 2.0 * M_PI / (27.321661 * 86400.0);
	moon.state.shape = BHS_SHAPE_SPHERE;

	/* Metadados */
	moon.type = BHS_BODY_MOON;  /* Satélite natural */
	moon.color = (struct bhs_vec3){0.7, 0.7, 0.7};  /* Cinza lunar */
	moon.is_alive = true;
	moon.is_fixed = false;

	/* Dados específicos */
	moon.prop.planet.density = 3344.0;           /* kg/m³ */
	moon.prop.planet.axis_tilt = 1.54 * M_PI / 180.0;  /* rad (mínima) */
	moon.prop.planet.albedo = 0.12;
	moon.prop.planet.has_atmosphere = false;
	moon.prop.planet.surface_pressure = 0;
	moon.prop.planet.atmosphere_mass = 0;
	strncpy(moon.prop.planet.composition, "Regolith, basalto", 
		sizeof(moon.prop.planet.composition));
	moon.prop.planet.temperature = 250.0;  /* K (média) */
	moon.prop.planet.physical_state = BHS_STATE_SOLID;
	moon.prop.planet.has_magnetic_field = false;

	printf("[PRESET] Lua criada: M=%.6f, R=%.2f, órbita=%.1f, v_rel=%.3f\n",
	       moon.state.mass, moon.state.radius, orbit_r, v_orb);

	return moon;
}

void bhs_preset_solar_system(bhs_scene_t scene)
{
	if (!scene) {
		fprintf(stderr, "[PRESET] Erro: cena nula\n");
		return;
	}

	printf("[PRESET] Criando Sistema Solar (Sol-Terra-Lua)...\n");

	/* Sol no centro */
	struct bhs_vec3 sun_pos = {0, 0, 0};
	struct bhs_body sun = bhs_preset_sun(sun_pos);
	bhs_scene_add_body(scene, sun.type, sun.state.pos, sun.state.vel,
			   sun.state.mass, sun.state.radius, sun.color);

	/* Terra em órbita */
	struct bhs_body earth = bhs_preset_earth(sun_pos);
	bhs_scene_add_body(scene, earth.type, earth.state.pos, earth.state.vel,
			   earth.state.mass, earth.state.radius, earth.color);

	/* Lua em órbita da Terra */
	struct bhs_body moon = bhs_preset_moon(earth.state.pos, earth.state.vel);
	bhs_scene_add_body(scene, moon.type, moon.state.pos, moon.state.vel,
			   moon.state.mass, moon.state.radius, moon.color);

	printf("[PRESET] Sistema Solar criado com 3 corpos!\n");
}
