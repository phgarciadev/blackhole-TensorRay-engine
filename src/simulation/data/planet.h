/**
 * @file planet.h
 * @brief Definições Universais para Corpos Planetários
 *
 * "A harmonia das esferas é matemática pura."
 *
 * Este arquivo define a interface comum para todos os planetas e corpos celestes
 * maiores que serão implementados como módulos individuais.
 */

#ifndef BHS_ENGINE_PLANETS_PLANET_H
#define BHS_ENGINE_PLANETS_PLANET_H

#include <stdbool.h>
#include "math/vec4.h"

/* ============================================================================
 * ENUMS
 * ============================================================================
 */

enum bhs_body_type_detail {
	BHS_PLANET_TERRESTRIAL, /* Rochoso */
	BHS_PLANET_GAS_GIANT,	/* Gigante Gasoso */
	BHS_PLANET_ICE_GIANT,	/* Gigante de Gelo */
	BHS_PLANET_DWARF,	/* Planeta Anão */
	BHS_STAR_MAIN_SEQ,	/* Estrela Sequência Principal */
	BHS_BLACK_HOLE		/* Singularidade */
};

/* ============================================================================
 * ESTRUTURA DE DADOS
 * ============================================================================
 */

/**
 * @brief Descritor Físico e Visual de um Planeta/Corpo
 */
struct bhs_planet_desc {
	/* Identificação */
	const char *name;
	enum bhs_body_type_detail type;

	/* Propriedades Físicas (SI) */
	double mass;		/* kg */
	double radius;		/* m (equatorial) */
	double density;		/* kg/m³ */
	double rotation_period; /* segundos (negativo = retrógrado) */
	double axis_tilt;	/* radianos */
	double gravity;		/* m/s² (superficial) */

	/* Orbital (Keplerianos básicos para setup inicial) */
	double semimajor_axis; /* m */
	double eccentricity;   /* 0..1 */
	double orbital_period; /* segundos */
    
	/* Keplerianos Completos (J2000) */
	double inclination;       /* graus (em relação à eclíptica) */
	double long_asc_node;     /* graus (Omega) */
	double long_perihelion;   /* graus (varpi) */
	double mean_longitude;    /* graus (L) */

	/* Atmosfera & Superfície */
	bool has_atmosphere;
	double surface_pressure; /* Pa */
	double mean_temperature; /* K */
	double albedo;		 /* 0..1 */

	/* Visual / Rendering "Drawing" Definition */
	struct bhs_vec3 base_color; /* Cor predominante */

	/* 
	 * Função procedural para "desenhar" a superfície
	 * Retorna a cor em um ponto normalizado da esfera (local space)
	 */
	struct bhs_vec3 (*get_surface_color)(struct bhs_vec3 p_local);
};

/* ============================================================================
 * PROTOTIPOS GLOBAIS (Factories)
 * ============================================================================
 */

/* Sol */
struct bhs_planet_desc bhs_sun_get_desc(void);

/* Planetas Rochosos */
struct bhs_planet_desc bhs_mercury_get_desc(void);
struct bhs_planet_desc bhs_venus_get_desc(void);
struct bhs_planet_desc bhs_earth_get_desc(void);
struct bhs_planet_desc bhs_mars_get_desc(void);

/* Gigantes */
struct bhs_planet_desc bhs_jupiter_get_desc(void);
struct bhs_planet_desc bhs_saturn_get_desc(void);
struct bhs_planet_desc bhs_uranus_get_desc(void);
struct bhs_planet_desc bhs_neptune_get_desc(void);

/* Outros */
struct bhs_planet_desc bhs_pluto_get_desc(void);
struct bhs_planet_desc bhs_moon_get_desc(void); /* Lua */
struct bhs_planet_desc bhs_ceres_get_desc(void);
struct bhs_planet_desc bhs_haumea_get_desc(void);
struct bhs_planet_desc bhs_makemake_get_desc(void);
struct bhs_planet_desc bhs_eris_get_desc(void);
struct bhs_planet_desc bhs_blackhole_get_desc(void);
struct bhs_planet_desc bhs_moon_get_desc(void);

/* ============================================================================
 * REGISTRY SYSTEM (Auto-discovery)
 * ============================================================================
 */

typedef struct bhs_planet_desc (*bhs_planet_getter_t)(void);

struct bhs_planet_registry_entry {
	const char *name;
	bhs_planet_getter_t getter;
	struct bhs_planet_registry_entry *next;
};

/**
 * @brief Register a planet module automatically at startup.
 */
void bhs_planet_register(const char *name, bhs_planet_getter_t getter);

/**
 * @brief Iterate over registered planets.
 * Returns NULL when no more planets are available.
 */
const struct bhs_planet_registry_entry *bhs_planet_registry_get_head(void);

#define BHS_REGISTER_PLANET(name_str, getter_func)                             \
	__attribute__((constructor)) static void                               \
	_register_planet_##getter_func(void)                                   \
	{                                                                      \
		bhs_planet_register(name_str, getter_func);                    \
	}

#endif /* BHS_ENGINE_PLANETS_PLANET_H */
