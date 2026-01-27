/*
 * @file sun.h
 * @brief Definição de Estrelas (Sóis)
 */

#ifndef BHS_ENGINE_DATA_SUN_H
#define BHS_ENGINE_DATA_SUN_H

#include "math/vec4.h"

/* Estágios evolutivos estelares */
enum bhs_sun_stage {
	BHS_SUN_MAIN_SEQUENCE,
	BHS_SUN_RED_GIANT,
	BHS_SUN_WHITE_DWARF,
	BHS_SUN_NEUTRON_STAR
};

/**
 * @brief Descritor Físico de uma Estrela
 */
struct bhs_sun_desc {
	const char *name;

	/* Física */
	double mass;	    /* kg */
	double radius;	    /* m */
	double temperature; /* K (Effective) */
	double luminosity;  /* W */
	double age;	    /* Anos */
	double metallicity; /* [Fe/H] (Log scale, Solar=0) */

	/* Classificação */
	enum bhs_sun_stage stage;
	char spectral_type[8]; /* Ex: "G2V" */

	/* Rotação */
	double rotation_period; /* s */
	double axis_tilt;	/* rad */

	/* Visual */
	struct bhs_vec3 base_color;

	/* Shader procedural de superfície */
	struct bhs_vec3 (*get_surface_color)(struct bhs_vec3 p_local);
};

#endif /* BHS_ENGINE_DATA_SUN_H */
