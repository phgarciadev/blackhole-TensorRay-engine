/*
 * @file blackhole.h
 * @brief Definição de Buracos Negros
 */

#ifndef BHS_ENGINE_DATA_BLACKHOLE_H
#define BHS_ENGINE_DATA_BLACKHOLE_H

#include "math/vec4.h"

/**
 * @brief Descritor Físico de um Buraco Negro
 */
struct bhs_blackhole_desc {
	const char *name;

	/* Física (No-Hair Theorem: Mass, Spin, Charge) */
	double mass;   /* kg */
	double spin;   /* a (Adimensional, 0..1) */
	double charge; /* Q (Geralmente 0 em astrofísica) */

	/* Derivados (para cache/facilidade) */
	double event_horizon_r;	    /* m (Schwarzschild ou Kerr radius) */
	double accretion_disk_mass; /* kg */

	/* Visual */
	struct bhs_vec3 base_color; /* Geralmente preto, mas útil para debug */

	/* Shader procedural (Geralmente retorna preto, mas pode ter disco de acreção visual) */
	struct bhs_vec3 (*get_surface_color)(struct bhs_vec3 p_local);
};

#endif /* BHS_ENGINE_DATA_BLACKHOLE_H */
