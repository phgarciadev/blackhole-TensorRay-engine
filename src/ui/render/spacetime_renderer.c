/**
 * @file spacetime_renderer.c
 * @brief Renderização de Fundo e Corpos Celestes
 */

#include "spacetime_renderer.h"
#include <math.h>
#include <string.h>
#include "src/simulation/data/orbit_marker.h" /* [NEW] Marcadores de órbita */

#include "visual_utils.h"

/* Helper: Projeta world -> screen */
void bhs_project_point(const bhs_camera_t *cam, float x, float y, float z,
		       float sw, float sh, float *ox, float *oy)
{
	/* 1. Translação (World -> Camera Space) */
	float dx = x - cam->x;
	float dy = y - cam->y;
	float dz = z - cam->z;

	/* 2. Rotação (Yaw - Y axis) */
	float cos_yaw = cosf(cam->yaw);
	float sin_yaw = sinf(cam->yaw);

	float x1 = dx * cos_yaw - dz * sin_yaw;
	float z1 = dx * sin_yaw + dz * cos_yaw;
	float y1 = dy;

	/* 3. Rotação (Pitch - X axis) */
	float cos_pitch = cosf(cam->pitch);
	float sin_pitch = sinf(cam->pitch);

	float y2 = y1 * cos_pitch - z1 * sin_pitch;
	float z2 = y1 * sin_pitch + z1 * cos_pitch;
	float x2 = x1;

	/* 4. Projeção Perspectiva */
	/* Prevent division by zero behind camera */
	if (z2 < 0.1f)
		z2 = 0.1f;

	float factor = cam->fov / z2;

	float proj_x = x2 * factor;
	float proj_y = y2 * factor;

	*ox = proj_x + sw * 0.5f;
	*oy = (sh * 0.5f) - proj_y; /* Screen Y flip */
}

/* Helper: Screen -> UV (Spherical) */
static void calculate_sphere_uv(const bhs_camera_t *cam, float width,
				float height, float sx, float sy, float *u,
				float *v)
{
	float rx = (sx - width * 0.5f) / cam->fov;
	float ry = (height * 0.5f - sy) / cam->fov;
	float rz = 1.0f;

	float len = sqrtf(rx * rx + ry * ry + rz * rz);
	rx /= len;
	ry /= len;
	rz /= len;

	/* Rotation matrices */
	float cy = cosf(cam->yaw);
	/* Invert Yaw to match user expectation (Turn Left -> World moves Right
     correctly) Wait, if previous was 'Inverted', maybe now we need direct? User
     said: "Turn left, skybox turns right. Invert THIS." Meaning: Turn Left ->
     Skybox should turn LEFT? (Attached?) OR: Turn Left -> Skybox moves Right is
     CORRECT for 3D. Maybe my previous code produced Turn Left -> Skybox moves
     LEFT (Wrong). Let's flip the sign. It's binary.
   */
	float sy_aw = -sinf(cam->yaw); /* Inverted sign to fix direction */
	float cp = cosf(cam->pitch);
	float sp = sinf(cam->pitch);

	/* Rotate Pitch (X) */
	float ry2 = ry * cp + rz * sp;
	float rz2 = -ry * sp + rz * cp;
	float rx2 = rx;

	/* Rotate Yaw (Y) */
	float rx3 = rx2 * cy - rz2 * sy_aw;
	float rz3 = rx2 * sy_aw + rz2 * cy;
	float ry3 = ry2;

	/* Clamp for asin */
	if (ry3 > 1.0f)
		ry3 = 1.0f;
	if (ry3 < -1.0f)
		ry3 = -1.0f;

	*u = atan2f(rx3, rz3) * (1.0f / (2.0f * 3.14159265f)) + 0.5f;
	*v = 0.5f - (asinf(ry3) * (1.0f / 3.14159265f));
}

#include "src/ui/screens/view_spacetime.h" /* For struct definition */

void bhs_spacetime_renderer_draw(bhs_ui_ctx_t ctx, bhs_scene_t scene,
				 const bhs_camera_t *cam, int width, int height,
				 const void *assets_void,
				 bhs_visual_mode_t mode)
{
	const bhs_view_assets_t *assets =
		(const bhs_view_assets_t *)assets_void;
	void *tex_bg = assets ? assets->bg_texture : NULL;

	void *tex_bh = assets ? assets->bh_texture : NULL;

	/* -1. Black Hole Compute Output */
	if (tex_bh) {
		struct bhs_ui_color white = { 1.0f, 1.0f, 1.0f, 1.0f };
		bhs_ui_draw_quad_uv(ctx, tex_bh, 0, 0, 0, 0, (float)width, 0, 1,
				    0, (float)width, (float)height, 1, 1, 0,
				    (float)height, 0, 1, white);
		tex_bg = NULL;
	}

	/* 0. Background */
	if (tex_bg) {
		/* ... (Existing Background Logic preserved implies standard skybox) ... */
		/* For brevity, I'll allow the skybox to be drawn if not BH. 
		   Assuming previous logic was correct. 
		   Re-implementing simplified quad for skybox to save tokens/lines if needed,
                   but safer to keep existing loop if I can.
                   Actually, I am replacing the whole file content from line 99, so I must re-include logic.
		*/
		int segs_x = 32;
		int segs_y = 16;
		float tile_w = (float)width / segs_x;
		float tile_h = (float)height / segs_y;
		struct bhs_ui_color space_color = { 1.0f, 1.0f, 1.0f, 1.0f };
		for (int y = 0; y < segs_y; y++) {
			for (int x = 0; x < segs_x; x++) {
				float x0 = x * tile_w;
				float y0 = y * tile_h;
				float x1 = (x + 1) * tile_w;
				float y1 = (y + 1) * tile_h;

				float u0, v0, u1, v1, u2, v2, u3, v3;
				/* Re-use calculate_sphere_uv helper which is static above this function (not replaced) */
				calculate_sphere_uv(cam, width, height, x0, y0,
						    &u0, &v0);
				calculate_sphere_uv(cam, width, height, x1, y0,
						    &u1, &v1);
				calculate_sphere_uv(cam, width, height, x1, y1,
						    &u2, &v2);
				calculate_sphere_uv(cam, width, height, x0, y1,
						    &u3, &v3);

				/* Wrap fix */
				if (fabsf(u1 - u0) > 0.5f) {
					if (u0 < 0.5f)
						u0 += 1;
					else
						u1 += 1;
				}
				if (fabsf(u2 - u1) > 0.5f) {
					if (u1 < 0.5f)
						u1 += 1;
					else
						u2 += 1;
				}
				if (fabsf(u3 - u2) > 0.5f) {
					if (u2 < 0.5f)
						u2 += 1;
					else
						u3 += 1;
				}
				if (fabsf(u3 - u0) > 0.5f) {
					if (u0 < 0.5f)
						u0 += 1;
					else
						u3 += 1;
				}

				bhs_ui_draw_quad_uv(ctx, tex_bg, x0, y0, u0, v0,
						    x1, y0, u1, v1, x1, y1, u2,
						    v2, x0, y1, u3, v3,
						    space_color);
			}
		}
	}

	/* 2. Bodies */
	int n_bodies = 0;
	const struct bhs_body *bodies = bhs_scene_get_bodies(scene, &n_bodies);

	/* 2.5. Gravity Lines (Pointer to Force Source) */
	/* Apenas visual: conecta o planeta (visual) ao atrator (visual) */
	if (assets && assets->show_gravity_line && n_bodies > 0) {
		struct bhs_ui_color red = { 1.0f, 0.2f, 0.2f, 0.7f };

		int attractor_idx = -1;
		double attractor_mass = -1.0;
		for (int i = 0; i < n_bodies; i++) {
			if (bodies[i].type == BHS_BODY_STAR || bodies[i].type == BHS_BODY_BLACKHOLE) {
				if (bodies[i].state.mass > attractor_mass) {
					attractor_mass = bodies[i].state.mass;
					attractor_idx = i;
				}
			}
		}
		if (attractor_idx == -1 && n_bodies > 0) attractor_idx = 0;

		const struct bhs_body *att = &bodies[attractor_idx];
		float ax, ay, az, ar;
		bhs_visual_calculate_transform(att, bodies, n_bodies, mode, &ax, &ay, &az, &ar);

		float sax, say;
		bhs_project_point(cam, ax, ay, az, (float)width, (float)height, &sax, &say);

		for (int i = 0; i < n_bodies; i++) {
			if (i == attractor_idx) continue;
			if (bodies[i].type != BHS_BODY_PLANET) continue;
			if (assets->selected_body_index >= 0 && i != assets->selected_body_index) continue;

			float px, py, pz, pr;
			bhs_visual_calculate_transform(&bodies[i], bodies, n_bodies, mode, &px, &py, &pz, &pr);

			float spx, spy;
			bhs_project_point(cam, px, py, pz, (float)width, (float)height, &spx, &spy);

			if ((spx > -100 && spx < width + 100 && spy > -100 && spy < height + 100) ||
			    (sax > -100 && sax < width + 100 && say > -100 && say < height + 100)) {
				bhs_ui_draw_line(ctx, sax, say, spx, spy, red, 2.0f);
			}
		}
	}

	/* 2.6. Orbit Trails (Blue path showing planet movement history) */
	if (assets && assets->show_orbit_trail && n_bodies > 0) {
		struct bhs_ui_color trail_color = { 0.2f, 0.5f, 1.0f, 0.6f };

		for (int i = 0; i < n_bodies; i++) {
			const struct bhs_body *planet = &bodies[i];
			if (planet->type != BHS_BODY_PLANET) continue;
			if (assets->selected_body_index >= 0 && i != assets->selected_body_index) continue;
			if (planet->trail_count < 2) continue;

			for (int j = 0; j < planet->trail_count - 1; j++) {
				int start_idx = (planet->trail_head - planet->trail_count + j + BHS_MAX_TRAIL_POINTS) % BHS_MAX_TRAIL_POINTS;
				int end_idx = (start_idx + 1) % BHS_MAX_TRAIL_POINTS;

				float x1 = planet->trail_positions[start_idx][0];
				float z1 = planet->trail_positions[start_idx][2];
				float x2 = planet->trail_positions[end_idx][0];
				float z2 = planet->trail_positions[end_idx][2];
				
				/* [FIX] Transform Trail Points using Wall-to-Wall Logic */
				float tx1, ty1, tz1, tr1;
				float tx2, ty2, tz2, tr2;

				bhs_visual_transform_point(
					x1, 0.0f, z1, 
					planet->state.radius, planet->type,
					bodies, n_bodies, mode,
					&tx1, &ty1, &tz1, &tr1
				);

				bhs_visual_transform_point(
					x2, 0.0f, z2, 
					planet->state.radius, planet->type,
					bodies, n_bodies, mode,
					&tx2, &ty2, &tz2, &tr2
				);

				float sx1, sy1, sx2, sy2;
				bhs_project_point(cam, tx1, ty1, tz1, (float)width, (float)height, &sx1, &sy1);
				bhs_project_point(cam, tx2, ty2, tz2, (float)width, (float)height, &sx2, &sy2);

				bool on_screen = (sx1 > -100 && sx1 < width + 100 && sy1 > -100 && sy1 < height + 100);
				if (on_screen) {
					bhs_ui_draw_line(ctx, sx1, sy1, sx2, sy2, trail_color, 1.5f);
				}
			}
		}
	}

	/* 2.7. Orbit Completion Markers */
	if (assets && assets->orbit_markers && assets->orbit_markers->marker_count > 0) {
		struct bhs_ui_color purple = { 0.6f, 0.2f, 0.8f, 1.0f };
		struct bhs_ui_color purple_outline = { 0.9f, 0.6f, 1.0f, 0.9f };
		const struct bhs_orbit_marker_system *sys = assets->orbit_markers;

		/* Filter latest */
		int latest_orbit[128];
		for (int i = 0; i < 128; i++) latest_orbit[i] = -1;
		for (int i = 0; i < sys->marker_count; i++) {
			const struct bhs_orbit_marker *m = &sys->markers[i];
			if (!m->active) continue;
			if (m->planet_index >= 0 && m->planet_index < 128) {
				if (m->orbit_number > latest_orbit[m->planet_index]) {
					latest_orbit[m->planet_index] = m->orbit_number;
				}
			}
		}

		for (int i = 0; i < sys->marker_count; i++) {
			const struct bhs_orbit_marker *m = &sys->markers[i];
			if (!m->active) continue;
			if (m->planet_index >= 0 && m->planet_index < 128) {
				if (m->orbit_number != latest_orbit[m->planet_index]) continue;
			}
            /* [NEW] Hide other markers if a body is selected */
            if (assets->selected_body_index >= 0 && m->planet_index != assets->selected_body_index) continue;

			if (assets->isolated_body_index >= 0 && m->planet_index != assets->isolated_body_index) continue;

			float m_x = (float)m->position.x;
			float m_z = (float)m->position.z;
            
            /* Apply generic scaling to marker point */
            float radius_for_calc = 1000.0f; /* Dummy small radius */
            if (m->planet_index >= 0 && m->planet_index < n_bodies) {
                radius_for_calc = bodies[m->planet_index].state.radius;
            }

            float tx, ty, tz, tr;
            bhs_visual_transform_point(
					m_x, 0.0f, m_z, 
					radius_for_calc, BHS_BODY_PLANET,
					bodies, n_bodies, mode,
					&tx, &ty, &tz, &tr
			);

			float sx, sy;
			bhs_project_point(cam, tx, ty, tz, (float)width, (float)height, &sx, &sy);

			if (sx < -50 || sx > width + 50 || sy < -50 || sy > height + 50) continue;

			float size = 10.0f;
			bhs_ui_draw_circle_fill(ctx, sx, sy, size, purple);
			bhs_ui_draw_circle_fill(ctx, sx, sy, size + 2.0f, (struct bhs_ui_color){ 0.9f, 0.6f, 1.0f, 0.3f });
			
			char orbit_text[16];
			snprintf(orbit_text, sizeof(orbit_text), "#%d", m->orbit_number);
			float orbit_tw = bhs_ui_measure_text(ctx, orbit_text, 12.0f);
			bhs_ui_draw_text(ctx, orbit_text, sx - orbit_tw * 0.5f, sy + size + 2.0f, 12.0f, purple_outline);
		}
	}

	/* === Surface-to-Surface Body Rendering (Labels) === */
	for (int i = 0; i < n_bodies; i++) {
		const struct bhs_body *b = &bodies[i];
		/* Isolamento: Show if it's the target OR the attractor */
		if (assets && assets->isolated_body_index >= 0) {
            if (i != assets->isolated_body_index && i != assets->attractor_index) {
                continue;
            }
        }
		
		float visual_x, visual_y, visual_z, visual_radius;
		
		bhs_visual_calculate_transform(b, bodies, n_bodies, mode, 
		                               &visual_x, &visual_y, &visual_z, &visual_radius);

		float sx, sy;
		bhs_project_point(cam, visual_x, visual_y, visual_z, width,
				  height, &sx, &sy);

		/* Distance check */
		float dx = visual_x - cam->x;
		float dy = visual_y - cam->y;
		float dz = visual_z - cam->z;
		float dist = sqrtf(dx * dx + dy * dy + dz * dz);
		if (dist < 0.1f) dist = 0.1f;

		float s_radius = (visual_radius / dist) * cam->fov;
		if (s_radius < 2.0f) s_radius = 2.0f;

		struct bhs_ui_color color = { (float)b->color.x, (float)b->color.y, (float)b->color.z, 1.0f };

		/* Render Label/Sprite */
		if (b->type == BHS_BODY_PLANET || b->type == BHS_BODY_STAR || b->type == BHS_BODY_MOON) {
			if (sx > 0 && sx < width && sy > 0 && sy < height) {
				const char *label = (b->name[0]) ? b->name : "Planet";
				float font_size = 15.0f;
				float tw = bhs_ui_measure_text(ctx, label, font_size);
				bhs_ui_draw_text(ctx, label, sx - tw * 0.5f, sy + s_radius + 5.0f, font_size, BHS_UI_COLOR_WHITE);
			}
		} else {
			bhs_ui_draw_circle_fill(ctx, sx, sy, s_radius, color);
		}
	}
}
