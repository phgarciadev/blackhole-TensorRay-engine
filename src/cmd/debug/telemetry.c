/**
 * @file telemetry.c
 * @brief Implementação do Dashboard de Debug
 */

#include "telemetry.h"
#include <stdio.h>
#include <math.h>

/* Helper para limpar tela ou voltar cursos (ANSI) */
static void clear_term(void)
{
	/* \033[2J clear screen, \033[H cursor home */
	/* Usar apenas Home para reduzir flickering se o terminal suportar */
	printf("\033[2J\033[H");
}

void bhs_telemetry_print_scene(bhs_scene_t scene, double time, bool show_grid)
{
	int count = 0;
	const struct bhs_body *bodies = bhs_scene_get_bodies(scene, &count);

	clear_term();
	printf("=== BLACK HOLE SIMULATOR - TELEMETRY (T=%.2fs) [Grid: %s] ===\n",
	       time, show_grid ? "ON" : "OFF");
	printf("Bodies: %d\n", count);
	printf("-----------------------------------------------------------------------------------------------------------------\n");
	printf("ID | Type    | Mass      | Radius | Pos (X, Z)          | Vel (X, Z)          | State / Prop\n");
	printf("---|---------|-----------|--------|---------------------|---------------------|------------------------------\n");

	for (int i = 0; i < count; i++) {
		const struct bhs_body *b = &bodies[i];
		char type_str[16];
		char extra_str[64];

		switch (b->type) {
		case BHS_BODY_PLANET:
			snprintf(type_str, sizeof(type_str), "Planet");
			snprintf(extra_str, sizeof(extra_str), "Dens=%.0f %s",
				 b->prop.planet.density,
				 (b->prop.planet.physical_state ==
				  BHS_STATE_SOLID) ?
					 "SOLID" :
					 "FLUID");
			break;
		case BHS_BODY_STAR:
			snprintf(type_str, sizeof(type_str), "Star");
			snprintf(extra_str, sizeof(extra_str), "Lum=%.1e Teff=%.0f",
				 b->prop.star.luminosity, b->prop.star.temp_effective);
			break;
		case BHS_BODY_BLACKHOLE:
			snprintf(type_str, sizeof(type_str), "BlackHole");
			snprintf(extra_str, sizeof(extra_str), "Spin=%.2f Rh=%.2f",
				 b->prop.bh.spin_factor, b->prop.bh.event_horizon_r);
			break;
		default:
			snprintf(type_str, sizeof(type_str), "Unknown");
			snprintf(extra_str, sizeof(extra_str), "-");
			break;
		}

		printf("%-2d | %-7s | %9.2f | %6.2f | (%7.2f, %7.2f) | (%7.3f, %7.3f) | %s\n",
		       i, type_str, b->state.mass, b->state.radius,
		       b->state.pos.x, b->state.pos.z,
		       b->state.vel.x, b->state.vel.z,
		       extra_str);
	}
	printf("-----------------------------------------------------------------------------------------------------------------\n");
}
