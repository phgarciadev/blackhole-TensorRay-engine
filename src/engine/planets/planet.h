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

#include "../../lib/math/vec4.h"
#include <stdbool.h>

/* ============================================================================
 * ENUMS
 * ============================================================================
 */

enum bhs_body_type_detail {
	BHS_PLANET_TERRESTRIAL, /* Rochoso */
	BHS_PLANET_GAS_GIANT,   /* Gigante Gasoso */
	BHS_PLANET_ICE_GIANT,   /* Gigante de Gelo */
	BHS_PLANET_DWARF,       /* Planeta Anão */
	BHS_STAR_MAIN_SEQ,      /* Estrela Sequência Principal */
	BHS_BLACK_HOLE          /* Singularidade */
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
	double mass;            /* kg */
	double radius;          /* m (equatorial) */
	double density;         /* kg/m³ */
	double rotation_period; /* segundos (negativo = retrógrado) */
	double axis_tilt;       /* radianos */
	double gravity;         /* m/s² (superficial) */

	/* Orbital (Keplerianos básicos para setup inicial) */
	double semimajor_axis;  /* m */
	double eccentricity;    /* 0..1 */
	double orbital_period;  /* segundos */

	/* Atmosfera & Superfície */
	bool has_atmosphere;
	double surface_pressure;/* Pa */
	double mean_temperature;/* K */
	double albedo;          /* 0..1 */

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
struct bhs_planet_desc bhs_ceres_get_desc(void);
struct bhs_planet_desc bhs_blackhole_get_desc(void);

#endif /* BHS_ENGINE_PLANETS_PLANET_H */
