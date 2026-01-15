/**
 * @file spacetime_renderer.c
 * @brief Renderização Pura da Malha (Projeção e Desenho de Linhas)
 */

#include "spacetime_renderer.h"
#include <math.h>
#include "engine/physics/spacetime/spacetime.h"

/* Helper: Projeta world -> screen */
/* Helper: Projeta world -> screen */
static void project_point(const bhs_camera_t *cam, float x, float y, float z,
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
				 const void *assets_void)
{
	const bhs_view_assets_t *assets =
		(const bhs_view_assets_t *)assets_void;
	void *tex_bg = assets ? assets->bg_texture : NULL;
	void *tex_sphere = assets ? assets->sphere_texture : NULL;

	/* 0. Background */
	if (tex_bg) {
		/* Higher tessellation for 'perfect' high-res rendering */
		int segs_x = 64;
		int segs_y = 32;

		float tile_w = (float)width / segs_x;
		float tile_h = (float)height / segs_y;

		/* Use full brightness, let the texture define the colors */
		struct bhs_ui_color space_color = { 1.0f, 1.0f, 1.0f, 1.0f };

		for (int y = 0; y < segs_y; y++) {
			for (int x = 0; x < segs_x; x++) {
				float x0 = x * tile_w;
				float x1 = (x + 1) * tile_w;
				float y0 = y * tile_h;
				float y1 = (y + 1) * tile_h;

				float u0, v0, u1, v1, u2, v2, u3, v3;

				calculate_sphere_uv(cam, (float)width,
						    (float)height, x0, y0, &u0,
						    &v0); // TL
				calculate_sphere_uv(cam, (float)width,
						    (float)height, x1, y0, &u1,
						    &v1); // TR
				calculate_sphere_uv(cam, (float)width,
						    (float)height, x1, y1, &u2,
						    &v2); // BR
				calculate_sphere_uv(cam, (float)width,
						    (float)height, x0, y1, &u3,
						    &v3); // BL

				/* Seam fix logic omitted for brevity, assuming standard u-wrapping */
				/* ... keeping existing seam logic ... */

				if (fabsf(u1 - u0) > 0.5f) {
					if (u0 < 0.5f)
						u0 += 1.0f;
					else
						u1 += 1.0f;
				}
				if (fabsf(u2 - u1) > 0.5f) {
					if (u1 < 0.5f)
						u1 += 1.0f;
					else
						u2 += 1.0f;
				}
				if (fabsf(u3 - u2) > 0.5f) {
					if (u2 < 0.5f)
						u2 += 1.0f;
					else
						u3 += 1.0f;
				}
				if (fabsf(u3 - u0) > 0.5f) {
					if (u0 < 0.5f)
						u0 += 1.0f;
					else
						u3 += 1.0f;
				}

				bhs_ui_draw_quad_uv(ctx, tex_bg, x0, y0, u0, v0,
						    x1, y0, u1, v1, x1, y1, u2,
						    v2, x0, y1, u3, v3,
						    space_color);
			}
		}
	}

	/* 1. Spacetime Grid (Wireframe) */
	/* Only draw if requested */
	if (assets && assets->show_grid) {
		bhs_spacetime_t st = bhs_scene_get_spacetime(scene);
		if (st) {
			float *vertices;
			int count;
			bhs_spacetime_get_render_data(st, &vertices, &count);
			if (vertices) {
				/* ... Rendering Loop (Skipping indentation for replace block clarity) ... */
				int divs = bhs_spacetime_get_divisions(st);
				int cols = divs + 1;
				int rows = divs + 1;

				struct bhs_ui_color col_base = { 0.0f, 0.8f,
								 1.0f, 0.3f };
				struct bhs_ui_color col_hilit = { 0.5f, 0.9f,
								  1.0f, 0.8f };

				for (int r = 0; r < rows; r++) {
					for (int c = 0; c < cols; c++) {
						int idx = (r * cols + c) * 6;

						float x1 = vertices[idx + 0];
						float y1 = vertices
							[idx +
							 1]; /* Deformação (Y físico) */
						float z1 = vertices[idx + 2];

						float sx1, sy1;
						project_point(cam, x1, y1, z1,
							      width, height,
							      &sx1, &sy1);

						bool deep = fabs(y1) > 0.5f;
						struct bhs_ui_color c1 =
							deep ? col_hilit
							     : col_base;
						float t1 = deep ? 2.0f : 1.0f;

						/* Horizontal (Right) */
						if (c < cols - 1) {
							int idx2 = (r * cols +
								    c + 1) *
								   6;
							float x2 =
								vertices[idx2 +
									 0];
							float y2 =
								vertices[idx2 +
									 1];
							float z2 =
								vertices[idx2 +
									 2];

							float sx2, sy2;
							project_point(
								cam, x2, y2, z2,
								width, height,
								&sx2, &sy2);

							bhs_ui_draw_line(
								ctx, sx1, sy1,
								sx2, sy2, c1,
								t1);
						}

						/* Vertical (Down) */
						if (r < rows - 1) {
							int idx2 = ((r +
								     1) * cols +
								    c) *
								   6;
							float x2 =
								vertices[idx2 +
									 0];
							float y2 =
								vertices[idx2 +
									 1];
							float z2 =
								vertices[idx2 +
									 2];

							float sx2, sy2;
							project_point(
								cam, x2, y2, z2,
								width, height,
								&sx2, &sy2);

							bhs_ui_draw_line(
								ctx, sx1, sy1,
								sx2, sy2, c1,
								t1);
						}
					}
				}
			}
		}
	}

	/* 2. Bodies (Planets, Black Holes) */
	int n_bodies = 0;
	const struct bhs_body *bodies = bhs_scene_get_bodies(scene, &n_bodies);

	for (int i = 0; i < n_bodies; i++) {
		const struct bhs_body *b = &bodies[i];

		float sx, sy;

		/* [VISUAL FIX] Calculate Visual Y for Planet
     * Physics runs on flat plane (Y=0) for Newtonian stability.
     * Grid runs on curved metric (Y = -M/r).
     * We snap the visual Rendering position to the Grid depth.
     */
		float visual_x = b->state.pos.x;
		float visual_y = b->state.pos.y;
		float visual_z = b->state.pos.z;

		if (b->type == BHS_BODY_PLANET &&
		    bodies[0].type == BHS_BODY_BLACKHOLE) {
			/* Assume body 0 is central mass */
			double dx = b->state.pos.x - bodies[0].state.pos.x;
			double dz = b->state.pos.z - bodies[0].state.pos.z;
			double r = sqrt(dx * dx + dz * dz);
			if (r > 1.0) {
				/* Match Grid Metric: -5.0 * (M/r) */
				visual_y = -5.0f * (float)(bodies[0].state.mass / r);
				if (visual_y < -50.0f)
					visual_y = -50.0f;
			}
		}

		/* Project body center */
		project_point(cam, visual_x, visual_y, visual_z, width, height,
			      &sx, &sy);

		/* Calculate screen radius based on distance */
		float dx = visual_x - cam->x;
		float dy = visual_y - cam->y;
		float dz = visual_z - cam->z;
		float dist = sqrtf(dx * dx + dy * dy + dz * dz);
		if (dist < 0.1f)
			dist = 0.1f;

		/* Project radius: size / dist * fov */
		
		/* 
		 * [VISUAL SCALE - PROPORTIONAL]
		 * 
		 * Raios físicos (unidades de simulação):
		 *   Sol:     3.00
		 *   Júpiter: 0.30
		 *   Saturno: 0.25
		 *   Terra:   0.027
		 *   Mercúrio: 0.011
		 * 
		 * Para que TUDO seja visível mas mantenha proporções REAIS,
		 * aplicamos magnificação UNIFORME a todos os corpos.
		 * 
		 * Fator 30x: Sol fica ~90, Júpiter ~9, Terra ~0.8
		 * Isso preserva: Sol > Júpiter > Saturno > Terra > Mercúrio
		 */
		float visual_radius = b->state.radius * 30.0f;

		float s_radius = (visual_radius / dist) * cam->fov;
		
		/* Mínimo visual de 2 pixels para que pontos não desapareçam */
		if (s_radius < 2.0f)
			s_radius = 2.0f;

		struct bhs_ui_color color = { (float)b->color.x,
					      (float)b->color.y,
					      (float)b->color.z, 1.0f };

		if (b->type == BHS_BODY_PLANET) {
			/* [IMPOSTRO 3D] Draw using generated sphere texture */
			/* FIX: Temporarily force fallback to circle for debugging/fixing visibility */
			if (tex_sphere && false) {
				float diameter = s_radius * 2.0f;
				/* DEBUG: Force NULL (White Texture) to verify Draw Call */
				bhs_ui_draw_texture(ctx, tex_sphere,
						    sx - s_radius,
						    sy - s_radius, diameter,
						    diameter, color);
					
					/* DEBUG LOG (Limit frequency) */
					static int dbg_count = 0;
					if (dbg_count++ % 120 == 0) {
						printf("[RENDER] Drawing Planet with Tex: %p\n", (void*)tex_sphere);
					}
			} else {
				/* Fallback if texture failed */
				bhs_ui_draw_circle_fill(ctx, sx, sy, s_radius,
							color);
			}

			/* Debug Label */
			const char *label = (b->name[0] != '\0') ? b->name : "Planet";
			bhs_ui_draw_text(ctx, label, sx + s_radius + 5, sy,
					 12.0f, BHS_UI_COLOR_WHITE);
		} else {
			/* Black Hole / Other: Keep standard circle */
			bhs_ui_draw_circle_fill(ctx, sx, sy, s_radius, color);
		}
	}
}
