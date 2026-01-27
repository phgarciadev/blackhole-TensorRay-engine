/**
 * @file gargantua.c
 * @brief Definição de Dados: Buraco Negro Supermassivo
 */

#include <string.h>
#include "src/simulation/data/planet.h"

const struct bhs_planet_desc gargantua_desc = {
	.name = "Gargantua",
	.type = BHS_BLACK_HOLE,
	.mass = 1.989e38,  /* 100 Milhões x Sol (aprox) */
	.radius = 2.95e11, /* ~2 AU Event Horizon */
	.base_color = { 0.0f, 0.0f, 0.0f },
	.has_atmosphere = false,
	/* Propriedades específicas de BH seriam passadas via components, 
       mas aqui usamos a struct genérica para init */
	.mean_temperature = 0.0f,
	.rotation_period = 0.0f /* Spin tratado separadamente */
};

/* Factory function se necessário ser mais dinâmica */
