/**
 * @file orbit_marker.c
 * @brief Implementação do Sistema de Detecção de Órbitas
 *
 * "Matemática pura, zero hardcode de períodos orbitais.
 *  Funciona até pra planetas inventados pelo usuário."
 */

#include "orbit_marker.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

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

	/* 1. Find System Attractor (heaviest Star/BH) for Hill Sphere Calc */
	int attractor_idx = -1;
	double max_sys_mass = 0;
	for (int i = 0; i < count; i++) {
		if ((bodies[i].type == BHS_BODY_STAR ||
		     bodies[i].type == BHS_BODY_BLACKHOLE) &&
		    bodies[i].state.mass > max_sys_mass) {
			max_sys_mass = bodies[i].state.mass;
			attractor_idx = i;
		}
	}
	if (attractor_idx < 0 && count > 0)
		attractor_idx = 0; /* Fallback */

	for (int i = 0; i < count && i < 128; i++) {
		/* Only Planets/Moons orbit stuff */
		if (bodies[i].type != BHS_BODY_PLANET &&
		    bodies[i].type !=
			    BHS_BODY_MOON) /* Support 'Moon' type if explicit */
			if (bodies[i].type != BHS_BODY_PLANET)
				continue; // Keep restrictive for now unless user re-tagged

		const struct bhs_body *me = &bodies[i];
		if (i == attractor_idx)
			continue;

		/* 2. Find My Parent (Hill Sphere Logic) */
		int parent_idx = attractor_idx;
		double best_score = 1.0e50;

		/* Search for better local parent */
		for (int j = 0; j < count; j++) {
			if (i == j)
				continue;
			if (bodies[j].state.mass <= me->state.mass)
				continue;

			double dx = me->state.pos.x - bodies[j].state.pos.x;
			double dy = me->state.pos.y - bodies[j].state.pos.y;
			double dz = me->state.pos.z - bodies[j].state.pos.z;
			double dist = sqrt(dx * dx + dy * dy + dz * dz);

			/* Calc Hill Radius of J relative to System Attractor */
			double hill_radius_j;
			if (j == attractor_idx) {
				hill_radius_j = 1.0e50;
			} else {
				/* R_H = a * cbrt(m/3M) */
				double dx_s = bodies[j].state.pos.x -
					      bodies[attractor_idx].state.pos.x;
				double dy_s = bodies[j].state.pos.y -
					      bodies[attractor_idx].state.pos.y;
				double dz_s = bodies[j].state.pos.z -
					      bodies[attractor_idx].state.pos.z;
				double dist_s = sqrt(dx_s * dx_s + dy_s * dy_s +
						     dz_s * dz_s);
				double mass_ratio = bodies[j].state.mass /
						    (3.0 * max_sys_mass);
				hill_radius_j =
					dist_s * pow(mass_ratio, 0.333333);
			}

			if (dist < hill_radius_j) {
				if (hill_radius_j < best_score) {
					best_score = hill_radius_j;
					parent_idx = j;
				}
			}
		}

		/* 3. Calculate Angle Relative to Parent */
		const struct bhs_body *parent = &bodies[parent_idx];
		double rel_x = me->state.pos.x - parent->state.pos.x;
		double rel_z = me->state.pos.z - parent->state.pos.z;
		double angle = atan2(rel_z, rel_x);

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

		/* Check for discontinuity (parent switch) */
		/* If angle jumped massively despite correction? No, just trust mechanics. 
		   Realistically if parent switches, 'prev_angle' might be wrong relative to new parent.
		   We should store 'last_parent_idx' in tracking to detect switch. 
		   For now, assume stability. */

		track->accumulated_angle += da;

		/* Detectar volta completa (2*PI radianos) */
		if (fabs(track->accumulated_angle) >= 2.0 * M_PI) {
			double period =
				current_time - track->last_crossing_time;

			/* Adiciona marcador */
			int idx = sys->marker_head;
			struct bhs_orbit_marker *m = &sys->markers[idx];

			m->active = true;
			m->planet_index = i;
			m->parent_index =
				parent_idx; /* [NEW] Store who we orbited */
			snprintf(m->planet_name, sizeof(m->planet_name), "%s",
				 me->name);
			m->timestamp_seconds = current_time;
			m->position = me->state.pos;
			m->orbit_number = ++track->orbit_count;
			m->orbital_period_measured = period;

			sys->marker_head =
				(sys->marker_head + 1) % BHS_MAX_ORBIT_MARKERS;
			if (sys->marker_count < BHS_MAX_ORBIT_MARKERS) {
				sys->marker_count++;
			}

			/* Logging less verbose or customized */
			// printf("[ORBIT] %s orbitou %s (#%d)\n", me->name, parent->name, track->orbit_count);

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
