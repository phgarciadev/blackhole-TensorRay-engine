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
		struct bhs_ui_color purple = { 0.8f, 0.4f, 1.0f, 0.8f }; /* [NEW] Purple for Moons */

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
			if (assets->selected_body_index >= 0 && i != assets->selected_body_index) continue;

			/* Skip logic if irrelevant */
			/* Agora tratamos todos os corpos iguais (exceto o próprio atrator) */
			if (bodies[i].type == BHS_BODY_BLACKHOLE) continue;

			/* Determine Target: Dynamic Hill Sphere Logic */
			/* "Quem é meu pai gravitacional LOCAL?" */
			
			int target_idx = attractor_idx;
			struct bhs_ui_color line_color = red;

			/* [NEW] Dynamic Classification: Is this a Moon? */
			/* Check who is the "Best Parent" using Hill Spheres */
			int dynamic_parent = bhs_visual_find_parent(i, bodies, n_bodies, NULL);
			
			/* If my parent is NOT the system attractor (Star/BH), I am a satellite (Moon). 
			   Moons should NOT show the Red Gravity Line to the Sun. 
			   They have their own Purple line logic below. */
			if (dynamic_parent != -1 && dynamic_parent != attractor_idx) {
				continue;
			}

			/* Hill Sphere Search */
			/* R_hill = a * cbrt(m / 3M) */
			/* Onde 'a' é a distância ao corpo maior M, m é a massa do corpo menor.
			   Aqui faremos o inverso: Estou dentro da esfera de Hill de alguém? */
			
			double best_parent_score = 1.0e50; /* Menor Hill Radius que me contém (mais local) */
			int best_parent = -1;

			for (int j = 0; j < n_bodies; j++) {
				if (i == j) continue;
				
				/* Pai deve ser mais massivo (regra prática para hierarquia estável) */
				if (bodies[j].state.mass <= bodies[i].state.mass) continue;
				
				/* Distância ao candidato a pai */
				double dx = bodies[i].state.pos.x - bodies[j].state.pos.x;
				double dy = bodies[i].state.pos.y - bodies[j].state.pos.y;
				double dz = bodies[i].state.pos.z - bodies[j].state.pos.z;
				double dist_sq = dx*dx + dy*dy + dz*dz;
				double dist = sqrt(dist_sq);
				
				/* Calcula Hill Sphere do candidato J em relação ao Atrator Supremo (Sol/BH) */
				/* R_H = Dist(J, Sun) * (M_j / 3 M_Sun)^(1/3) */
				/* Se J for o próprio atrator, R_H é infinito. */
				
				double hill_radius_j;
				if (j == attractor_idx) {
					hill_radius_j = 1.0e50; /* Infinito */
				} else {
					double dx_s = bodies[j].state.pos.x - bodies[attractor_idx].state.pos.x;
					double dy_s = bodies[j].state.pos.y - bodies[attractor_idx].state.pos.y;
					double dz_s = bodies[j].state.pos.z - bodies[attractor_idx].state.pos.z;
					double dist_sun = sqrt(dx_s*dx_s + dy_s*dy_s + dz_s*dz_s);
					
					double mass_ratio = bodies[j].state.mass / (3.0 * attractor_mass);
					hill_radius_j = dist_sun * pow(mass_ratio, 0.333333);
				}
				
				/* Teste: Estou dentro da Hill Sphere de J? */
				if (dist < hill_radius_j) {
					/* Sim! J é um pai possível. */
					/* Queremos o pai "mais local", ou seja, aquele com MENOR Hill Radius 
					   que ainda me contém. Ex: Lua está na Hill da Terra (raio pqno) e do Sol (raio enorme).
					   Terra ganha. */
					if (hill_radius_j < best_parent_score) {
						best_parent_score = hill_radius_j;
						best_parent = j;
					}
				}
			}

			if (best_parent != -1 && best_parent != attractor_idx) {
				target_idx = best_parent;
				line_color = purple;
			}

			/* Calculate positions */
			float px, py, pz, pr;
			bhs_visual_calculate_transform(&bodies[i], bodies, n_bodies, mode, &px, &py, &pz, &pr);

			float spx, spy;
			bhs_project_point(cam, px, py, pz, (float)width, (float)height, &spx, &spy);
			
			/* Calculate Target Position */
			/* Reuse `sax`/`say` if target is attractor, else recalc */
			float stx, sty;
			if (target_idx == attractor_idx) {
				stx = sax; sty = say;
			} else {
				const struct bhs_body *tgt = &bodies[target_idx];
				float tx, ty, tz, tr;
				bhs_visual_calculate_transform(tgt, bodies, n_bodies, mode, &tx, &ty, &tz, &tr);
				bhs_project_point(cam, tx, ty, tz, (float)width, (float)height, &stx, &sty);
			}

			/* Draw if visible on screen (approx) */
			if ((spx > -100 && spx < width + 100 && spy > -100 && spy < height + 100) ||
			    (stx > -100 && stx < width + 100 && sty > -100 && sty < height + 100)) {
				bhs_ui_draw_line(ctx, stx, sty, spx, spy, line_color, 2.0f);
			}
		}
	}

	/* 2.6. [NEW] Satellite Orbits Visualizer (Green + Purple) */
	/* Separate Toggle for Moon Analysis as requested */
	if (assets && assets->show_satellite_orbits && n_bodies > 0) {
		struct bhs_ui_color purple = { 0.8f, 0.4f, 1.0f, 0.8f };

		/* Find System Attractor (Sun) just to exclude it */
		int attractor_idx = -1;
		double max_mass = -1.0;
		for (int i = 0; i < n_bodies; i++) {
			if (bodies[i].type == BHS_BODY_STAR || bodies[i].type == BHS_BODY_BLACKHOLE) {
				if (bodies[i].state.mass > max_mass) {
					max_mass = bodies[i].state.mass;
					attractor_idx = i;
				}
			}
		}
		if (attractor_idx == -1 && n_bodies > 0) attractor_idx = 0;

		for (int i = 0; i < n_bodies; i++) {
			if (i == attractor_idx) continue; /* Sun has no parent */
			if (assets->selected_body_index >= 0 && i != assets->selected_body_index) continue;
			
			/* Dynamic Parent Detection (Hill Sphere) */
			double best_score = 1.0e50;
			int parent_idx = -1;

			for (int j = 0; j < n_bodies; j++) {
				if (i == j) continue;
				if (bodies[j].state.mass <= bodies[i].state.mass) continue;

				double dx = bodies[i].state.pos.x - bodies[j].state.pos.x;
				double dy = bodies[i].state.pos.y - bodies[j].state.pos.y;
				double dz = bodies[i].state.pos.z - bodies[j].state.pos.z;
				double dist = sqrt(dx*dx + dy*dy + dz*dz);

				double hill_radius_j;
				if (j == attractor_idx) {
					hill_radius_j = 1.0e50; 
				} else {
					double dx_s = bodies[j].state.pos.x - bodies[attractor_idx].state.pos.x;
					double dy_s = bodies[j].state.pos.y - bodies[attractor_idx].state.pos.y;
					double dz_s = bodies[j].state.pos.z - bodies[attractor_idx].state.pos.z;
					double dist_sun = sqrt(dx_s*dx_s + dy_s*dy_s + dz_s*dz_s);
					double mass_ratio = bodies[j].state.mass / (3.0 * max_mass);
					hill_radius_j = dist_sun * pow(mass_ratio, 0.333333);
				}

				if (dist < hill_radius_j) {
					if (hill_radius_j < best_score) {
						best_score = hill_radius_j;
						parent_idx = j;
					}
				}
			}
			
			/* Only if parent is NOT the Sun (meaning it's a Mooney orbit) */
			if (parent_idx != -1 && parent_idx != attractor_idx) {
				/* Draw Purple Line (Link to Parent) */
				float px, py, pz, pr;
				bhs_visual_calculate_transform(&bodies[i], bodies, n_bodies, mode, &px, &py, &pz, &pr);
				
				float tx, ty, tz, tr;
				bhs_visual_calculate_transform(&bodies[parent_idx], bodies, n_bodies, mode, &tx, &ty, &tz, &tr);
				
				float sx1, sy1, sx2, sy2;
				bhs_project_point(cam, px, py, pz, (float)width, (float)height, &sx1, &sy1);
				bhs_project_point(cam, tx, ty, tz, (float)width, (float)height, &sx2, &sy2);
				bhs_ui_draw_line(ctx, sx1, sy1, sx2, sy2, purple, 1.5f);
				
				/* Draw Green Line (Orbit Trail Relative to Parent) */
				/* "Rastro de Lesma" - Path history relative to parent */
				
				/* Draw Green Line (Absolute Spirograph Path - Visually Anchored) */
				/* "Rastro de Lesma" - O caminho real percorrido, mas attached ao sistema visual. */
				
				const struct bhs_body *parent = &bodies[parent_idx];
				int history_len = bodies[i].trail_count;
				if (parent->trail_count < history_len) history_len = parent->trail_count;

				struct bhs_ui_color green_trail = { 0.2f, 1.0f, 0.2f, 0.6f };

				/* Pre-calc Parent Radius Scaling for optimization */
				float rad_mult_parent = (mode == BHS_VISUAL_MODE_CINEMATIC) ? 1200.0f : 1.0f;
				if (mode == BHS_VISUAL_MODE_DIDACTIC) rad_mult_parent = 1200.0f;
				float parent_vis_radius = (float)parent->state.radius * rad_mult_parent;
				if (parent->type == BHS_BODY_STAR && mode != BHS_VISUAL_MODE_SCIENTIFIC) {
					parent_vis_radius = (float)parent->state.radius * 100.0f;
				}

				float rad_mult_moon = (mode == BHS_VISUAL_MODE_CINEMATIC) ? 1200.0f : 1.0f;
				if (mode == BHS_VISUAL_MODE_DIDACTIC) rad_mult_moon = 1200.0f;
				float moon_vis_radius = (float)bodies[i].state.radius * rad_mult_moon;

				/* Iterate Points */
				for (int k = 1; k < history_len; k++) {
					int idx_curr = (bodies[i].trail_head - k + BHS_MAX_TRAIL_POINTS) % BHS_MAX_TRAIL_POINTS;
					int idx_prev = (bodies[i].trail_head - (k+1) + BHS_MAX_TRAIL_POINTS) % BHS_MAX_TRAIL_POINTS;
					
					int p_idx_curr = (parent->trail_head - k + BHS_MAX_TRAIL_POINTS) % BHS_MAX_TRAIL_POINTS;
					int p_idx_prev = (parent->trail_head - (k+1) + BHS_MAX_TRAIL_POINTS) % BHS_MAX_TRAIL_POINTS;
					
					/* 1. Calculate Historical PARENT Visual Position */
					/* We assume Parent's parent is ROOT/Sun (Stable at 0,0,0 usually). */
					/* If Parent moves relative to Sun, transform(Parent_Hist) handles it. */
					/* Trick: call transform on Parent_Hist but force index = PARENT_IDX */
					
					double px_curr = parent->trail_positions[p_idx_curr][0];
					double py_curr = parent->trail_positions[p_idx_curr][1];
					double pz_curr = parent->trail_positions[p_idx_curr][2];
					
					float p_vis_x1, p_vis_y1, p_vis_z1, p_vis_r1;
					bhs_visual_transform_point(px_curr, py_curr, pz_curr, 
											   parent->state.radius, parent->type,
											   bodies, n_bodies, mode, 
											   parent_idx, /* FORCE Index to use Parent Scaling Rules */
											   &p_vis_x1, &p_vis_y1, &p_vis_z1, &p_vis_r1);

					double px_prev = parent->trail_positions[p_idx_prev][0];
					double py_prev = parent->trail_positions[p_idx_prev][1];
					double pz_prev = parent->trail_positions[p_idx_prev][2];

					float p_vis_x2, p_vis_y2, p_vis_z2, p_vis_r2;
					bhs_visual_transform_point(px_prev, py_prev, pz_prev, 
											   parent->state.radius, parent->type,
											   bodies, n_bodies, mode, 
											   parent_idx,
											   &p_vis_x2, &p_vis_y2, &p_vis_z2, &p_vis_r2);

					/* 2. Manually Calculate MOON Visual Position attached to that Parent */
					/* Logic from visual_utils: VisPos = ParentVis + Dir * (ParentR + MoonR + Gap) */
					
					/* Point 1 */
					double mx_curr = bodies[i].trail_positions[idx_curr][0];
					double my_curr = bodies[i].trail_positions[idx_curr][1];
					double mz_curr = bodies[i].trail_positions[idx_curr][2];
					
					double dx1 = mx_curr - px_curr;
					double dy1 = my_curr - py_curr;
					double dz1 = mz_curr - pz_curr;
					double dist1 = sqrt(dx1*dx1 + dy1*dy1 + dz1*dz1);
					if (dist1 < 1.0) dist1 = 1.0;
					
					double gap1 = dist1 - parent->state.radius - bodies[i].state.radius;
					if (gap1 < 0) gap1 = 0;
					/* In Cinematic, gap is usually 1:1, just radii scaled. */
					/* Actually gap_scale = 1.0f in utils. */
					double vis_dist1 = parent_vis_radius + moon_vis_radius + gap1;
					
					float m_vis_x1 = p_vis_x1 + (float)((dx1/dist1) * vis_dist1);
					float m_vis_y1 = p_vis_y1 + (float)((dy1/dist1) * vis_dist1); 
					float m_vis_z1 = p_vis_z1 + (float)((dz1/dist1) * vis_dist1);

					/* Point 2 */
					double mx_prev = bodies[i].trail_positions[idx_prev][0];
					double my_prev = bodies[i].trail_positions[idx_prev][1];
					double mz_prev = bodies[i].trail_positions[idx_prev][2];
					
					double dx2 = mx_prev - px_prev;
					double dy2 = my_prev - py_prev;
					double dz2 = mz_prev - pz_prev;
					double dist2 = sqrt(dx2*dx2 + dy2*dy2 + dz2*dz2);
					if (dist2 < 1.0) dist2 = 1.0;
					
					double gap2 = dist2 - parent->state.radius - bodies[i].state.radius;
					if (gap2 < 0) gap2 = 0;
					double vis_dist2 = parent_vis_radius + moon_vis_radius + gap2;
					
					float m_vis_x2 = p_vis_x2 + (float)((dx2/dist2) * vis_dist2);
					float m_vis_y2 = p_vis_y2 + (float)((dy2/dist2) * vis_dist2); 
					float m_vis_z2 = p_vis_z2 + (float)((dz2/dist2) * vis_dist2);

					/* Project */
					float gx1, gy1, gx2, gy2;
					bhs_project_point(cam, m_vis_x1, m_vis_y1, m_vis_z1, (float)width, (float)height, &gx1, &gy1);
					bhs_project_point(cam, m_vis_x2, m_vis_y2, m_vis_z2, (float)width, (float)height, &gx2, &gy2);
					
					/* Cull off-screen? */
					if (gx1 > -50 && gx1 < width+50 && gy1 > -50 && gy1 < height+50) {
						bhs_ui_draw_line(ctx, gx1, gy1, gx2, gy2, green_trail, 1.5f);
					}
				}
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
			
			/* [NEW] Dynamic Classification: Is this a Moon? */
			/* If so, suppress Blue Trail (Planetary Orbit). It gets a Green Trail elsewhere. */
			int dynamic_parent = bhs_visual_find_parent(i, bodies, n_bodies, NULL);
			
			/* Find Attractor Index again (Duplicate logic, could optimize but safe) */
			int attractor_idx = -1;
			double max_mass = -1.0;
			for (int k = 0; k < n_bodies; k++) {
				if (bodies[k].type == BHS_BODY_STAR || bodies[k].type == BHS_BODY_BLACKHOLE) {
					if (bodies[k].state.mass > max_mass) {
						max_mass = bodies[k].state.mass;
						attractor_idx = k;
					}
				}
			}
			if (attractor_idx == -1 && n_bodies > 0) attractor_idx = 0;

			if (dynamic_parent != -1 && dynamic_parent != attractor_idx) {
				/* I am a moon! Skip Blue Trail */
				continue;
			}

			if (planet->trail_count < 2) continue;

			for (int j = 0; j < planet->trail_count - 1; j++) {
				int start_idx = (planet->trail_head - planet->trail_count + j + BHS_MAX_TRAIL_POINTS) % BHS_MAX_TRAIL_POINTS;
				int end_idx = (start_idx + 1) % BHS_MAX_TRAIL_POINTS;

				float x1 = planet->trail_positions[start_idx][0];
				float y1 = planet->trail_positions[start_idx][1];
				float z1 = planet->trail_positions[start_idx][2];
				float x2 = planet->trail_positions[end_idx][0];
				float y2 = planet->trail_positions[end_idx][1];
				float z2 = planet->trail_positions[end_idx][2];
				
				/* [FIX] Transform Trail Points using Wall-to-Wall Logic */
				float tx1, ty1, tz1, tr1;
				float tx2, ty2, tz2, tr2;

				bhs_visual_transform_point(
					x1, y1, z1, 
					planet->state.radius, planet->type,
					bodies, n_bodies, mode,
					i, /* FORCE: I am part of planet[i]'s history! */
					&tx1, &ty1, &tz1, &tr1
				);

				bhs_visual_transform_point(
					x2, y2, z2, 
					planet->state.radius, planet->type,
					bodies, n_bodies, mode,
					i, /* FORCE: I am part of planet[i]'s history! */
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
					m->planet_index, /* FORCE: Marker attached to this planet */
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
