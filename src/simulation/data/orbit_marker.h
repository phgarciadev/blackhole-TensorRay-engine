/**
 * @file orbit_marker.h
 * @brief Sistema de Marcadores de Órbita Completa
 *
 * "Quando um planeta der uma volta completa, eu quero saber. 
 *  Não com hardcode de período, mas com matemática de verdade."
 *
 * Detecção baseada em cruzamento angular (theta = atan2).
 * Funciona pra qualquer planeta, inclusive elípticos.
 */

#ifndef BHS_ORBIT_MARKER_H
#define BHS_ORBIT_MARKER_H

#include <stdbool.h>
#include "math/bhs_math.h"
#include "engine/scene/scene.h"

#define BHS_MAX_ORBIT_MARKERS 64

/**
 * struct bhs_orbit_marker - Um evento de órbita completa
 *
 * Invariantes:
 * - active == true indica marcador válido
 * - timestamp_seconds >= 0 (segundos desde J2000)
 */
struct bhs_orbit_marker {
	bool active;
	int planet_index;		/* Índice do planeta que completou */
	int parent_index;       /* [NEW] Índice do corpo orbitado */
	char planet_name[32];		/* Nome pro display */
	double timestamp_seconds;	/* Tempo exato desde J2000 */
	struct bhs_vec3 position;	/* Posição no momento da conclusão */
	int orbit_number;		/* Qual órbita (1ª, 2ª, etc) */
	double orbital_period_measured; /* Período medido desta órbita em segundos */
};

/**
 * struct bhs_orbit_tracking - Estado de tracking por planeta
 */
struct bhs_orbit_tracking {
	double prev_angle;		/* Ângulo θ anterior */
	double accumulated_angle;	/* [NEW] Soma dos deltas angulares (rad) */
	double last_crossing_time;	/* Tempo da última volta */
	int orbit_count;		/* Contador de órbitas */
	bool initialized;
};

/**
 * struct bhs_orbit_marker_system - Container global de marcadores
 *
 * Invariantes:
 * - marker_count <= BHS_MAX_ORBIT_MARKERS
 * - tracking[i] corresponde ao body de índice i
 */
struct bhs_orbit_marker_system {
	struct bhs_orbit_marker markers[BHS_MAX_ORBIT_MARKERS];
	int marker_count;
	int marker_head;		/* Buffer circular */

	struct bhs_orbit_tracking tracking[128]; /* MAX_BODIES */
};

/**
 * bhs_orbit_markers_init - Inicializa sistema de marcadores
 */
void bhs_orbit_markers_init(struct bhs_orbit_marker_system *sys);

/**
 * bhs_orbit_markers_update - Detecta novas órbitas completas
 * @sys: Sistema de marcadores
 * @bodies: Array de corpos da cena
 * @count: Número de corpos
 * @current_time: Tempo atual da simulação (segundos desde J2000)
 *
 * Usa detecção por cruzamento angular. Quando o ângulo polar
 * de um planeta em relação ao Sol passa de +π para -π (ou vice-versa),
 * uma órbita completa é registrada.
 */
void bhs_orbit_markers_update(struct bhs_orbit_marker_system *sys,
			      const struct bhs_body *bodies, int count,
			      double current_time);

/**
 * bhs_orbit_markers_get_at_screen - Hit test para clique em marcador
 * @returns: Índice do marcador clicado ou -1 se nenhum
 */
int bhs_orbit_markers_get_at_screen(const struct bhs_orbit_marker_system *sys,
				    float screen_x, float screen_y,
				    const void *cam, int width, int height);

#endif /* BHS_ORBIT_MARKER_H */
