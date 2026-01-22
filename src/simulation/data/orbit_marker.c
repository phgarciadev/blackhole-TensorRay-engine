/**
 * @file orbit_marker.c
 * @brief Implementação do Sistema de Detecção de Órbitas
 *
 * "Matemática pura, zero hardcode de períodos orbitais.
 *  Funciona até pra planetas inventados pelo usuário."
 */

#include "orbit_marker.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

/* PI, porque ninguém lembra de cabeça */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void bhs_orbit_markers_init(struct bhs_orbit_marker_system *sys)
{
	if (!sys)
		return;
	memset(sys, 0, sizeof(*sys));
}

void bhs_orbit_markers_update(struct bhs_orbit_marker_system *sys,
			      const struct bhs_body *bodies, int count,
			      double current_time)
{
	if (!sys || !bodies || count <= 0)
		return;

	/* Encontrar o Sol (maior massa com tipo STAR) */
	int sun_idx = -1;
	double max_mass = 0;
	for (int i = 0; i < count; i++) {
		if (bodies[i].type == BHS_BODY_STAR &&
		    bodies[i].state.mass > max_mass) {
			max_mass = bodies[i].state.mass;
			sun_idx = i;
		}
	}

	/* Sem sol, sem órbitas (faz sentido, né?) */
	if (sun_idx < 0)
		return;

	const struct bhs_body *sun = &bodies[sun_idx];

	for (int i = 0; i < count && i < 128; i++) {
		/* Só planetas orbitam (por enquanto) */
		if (bodies[i].type != BHS_BODY_PLANET)
			continue;

		const struct bhs_body *planet = &bodies[i];

		/* Calcular ângulo polar relativo ao Sol */
		double dx = planet->state.pos.x - sun->state.pos.x;
		double dz = planet->state.pos.z - sun->state.pos.z;
		double angle = atan2(dz, dx); /* -π a +π */

		struct bhs_orbit_tracking *track = &sys->tracking[i];

		/* Inicializar tracking se necessário */
		if (!track->initialized) {
			track->prev_angle = angle;
			track->accumulated_angle = 0.0;
			track->last_crossing_time = current_time;
			track->initialized = true;
			continue;
		}

		/* Calcular delta angular corrigindo wrap-around do atan2 */
		double da = angle - track->prev_angle;
		if (da > M_PI)
			da -= 2.0 * M_PI;
		if (da < -M_PI)
			da += 2.0 * M_PI;

		track->accumulated_angle += da;

		/* 
		 * Detectar volta completa (2*PI radianos) 
		 * Suporta órbitas horárias e anti-horárias.
		 */
		if (fabs(track->accumulated_angle) >= 2.0 * M_PI) {
			/* Calcula período real desta órbita */
			double period = current_time - track->last_crossing_time;

			/* Adiciona marcador (buffer circular) */
			int idx = sys->marker_head;
			struct bhs_orbit_marker *m = &sys->markers[idx];

			m->active = true;
			m->planet_index = i;
			snprintf(m->planet_name, sizeof(m->planet_name), "%s",
				 planet->name);
			m->timestamp_seconds = current_time;
			m->position = planet->state.pos;
			m->orbit_number = ++track->orbit_count;
			m->orbital_period_measured = period;

			/* Avança head do buffer circular */
			sys->marker_head =
				(sys->marker_head + 1) % BHS_MAX_ORBIT_MARKERS;
			if (sys->marker_count < BHS_MAX_ORBIT_MARKERS) {
				sys->marker_count++;
			}

			/* Log pra debug */
			printf("[ORBIT] %s completou órbita #%d! Período: %.2f dias "
			       "(Matemático: 2*PI accum)\n",
			       planet->name, track->orbit_count,
			       period / 86400.0);

			/* Reset pra próxima volta, mantendo residual */
			if (track->accumulated_angle > 0)
				track->accumulated_angle -= 2.0 * M_PI;
			else
				track->accumulated_angle += 2.0 * M_PI;

			track->last_crossing_time = current_time;
		}

		track->prev_angle = angle;
	}
}

/*
 * Hit test - retorna índice do marcador sob o cursor
 * Precisa da função de projeção, mas como é interna ao renderer...
 * essa função vai ter que ser chamada de lá. Deixo placeholder.
 */
#include "src/ui/render/spacetime_renderer.h"

int bhs_orbit_markers_get_at_screen(const struct bhs_orbit_marker_system *sys,
				    float screen_x, float screen_y,
				    const void *cam_void, int width, int height)
{
	if (!sys || !cam_void)
		return -1;

	const bhs_camera_t *cam = (const bhs_camera_t *)cam_void;
	float threshold = 15.0f; /* Distância de clique em pixels */

	for (int i = 0; i < sys->marker_count; i++) {
		const struct bhs_orbit_marker *m = &sys->markers[i];
		if (!m->active)
			continue;

		float sx, sy;
		bhs_project_point(cam, (float)m->position.x, 0.0f,
				 (float)m->position.z, (float)width,
				 (float)height, &sx, &sy);

		float dx = screen_x - sx;
		float dy = screen_y - sy;
		float dist_sq = dx * dx + dy * dy;

		if (dist_sq < threshold * threshold) {
			return i;
		}
	}

	return -1;
}
