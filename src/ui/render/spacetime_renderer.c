/**
 * @file spacetime_renderer.c
 * @brief Renderização de Fundo e Corpos Celestes
 */

#include "spacetime_renderer.h"
#include <math.h>
#include <string.h>
#include "src/simulation/data/orbit_marker.h" /* [NEW] Marcadores de órbita */

/* Helper: Projeta world -> screen */
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

/* Helper: Calculate gravity depth for object placement */
static float calculate_gravity_depth(float x, float z, const struct bhs_body *bodies, int n_bodies)
{
	if (!bodies || n_bodies == 0) return 0.0f;
	
	float potential = 0.0f;
	for (int i = 0; i < n_bodies; i++) {
		float dx = x - bodies[i].state.pos.x;
		float dz = z - bodies[i].state.pos.z;
		
		float dist_sq = dx*dx + dz*dz;
		float dist = sqrtf(dist_sq + 0.1f);
		
		double eff_mass = bodies[i].state.mass;
		if (bodies[i].type == BHS_BODY_PLANET) {
			eff_mass *= 5000.0; /* Visual boost for planet gravity wells */
		}
		
		potential -= (eff_mass) / dist; /* G=1 visual */
	}
	float depth = potential * 5.0f; /* Potential scale */
	if (depth < -50.0f) depth = -50.0f;
	return depth;
}

void bhs_spacetime_renderer_draw(bhs_ui_ctx_t ctx, bhs_scene_t scene,
				 const bhs_camera_t *cam, int width, int height,
				 const void *assets_void)
{
	const bhs_view_assets_t *assets = (const bhs_view_assets_t *)assets_void;
	void *tex_bg = assets ? assets->bg_texture : NULL;

	void *tex_bh = assets ? assets->bh_texture : NULL;

	/* -1. Black Hole Compute Output */
	if (tex_bh) {
		struct bhs_ui_color white = { 1.0f, 1.0f, 1.0f, 1.0f };
		bhs_ui_draw_quad_uv(ctx, tex_bh,
				    0, 0, 0, 0,
				    (float)width, 0, 1, 0,
				    (float)width, (float)height, 1, 1,
				    0, (float)height, 0, 1,
				    white);
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
        int segs_x = 32; int segs_y = 16;
        float tile_w = (float)width / segs_x;
        float tile_h = (float)height / segs_y;
        struct bhs_ui_color space_color = { 1.0f, 1.0f, 1.0f, 1.0f };
        for (int y = 0; y < segs_y; y++) {
            for (int x = 0; x < segs_x; x++) {
                float x0 = x * tile_w; float y0 = y * tile_h;
                float x1 = (x+1) * tile_w; float y1 = (y+1) * tile_h;
                
                float u0,v0, u1,v1, u2,v2, u3,v3;
				/* Re-use calculate_sphere_uv helper which is static above this function (not replaced) */
                calculate_sphere_uv(cam, width, height, x0, y0, &u0, &v0);
                calculate_sphere_uv(cam, width, height, x1, y0, &u1, &v1);
                calculate_sphere_uv(cam, width, height, x1, y1, &u2, &v2);
                calculate_sphere_uv(cam, width, height, x0, y1, &u3, &v3);
                
                /* Wrap fix */
                if(fabsf(u1-u0)>0.5f) { if(u0<0.5f) u0+=1; else u1+=1; }
                if(fabsf(u2-u1)>0.5f) { if(u1<0.5f) u1+=1; else u2+=1; }
                if(fabsf(u3-u2)>0.5f) { if(u2<0.5f) u2+=1; else u3+=1; }
                if(fabsf(u3-u0)>0.5f) { if(u0<0.5f) u0+=1; else u3+=1; }

                bhs_ui_draw_quad_uv(ctx, tex_bg, x0, y0, u0, v0, x1, y0, u1, v1, x1, y1, u2, v2, x0, y1, u3, v3, space_color);
            }
        }
	}

	/* 2. Bodies */
	int n_bodies = 0;
	const struct bhs_body *bodies = bhs_scene_get_bodies(scene, &n_bodies);

	/* 2.5. Gravity Lines (Drawn on top of background, behind body labels) */
	if (assets && assets->show_gravity_line && n_bodies > 0) {
		/* 
		 * Lógica: 
		 * - Se selected_body_index == -1: desenha linhas de TODOS os planetas pro atrator
		 * - Se selected_body_index >= 0: desenha SÓ a linha do planeta selecionado
		 */
		
		/* Primeiro, encontra o atrator principal (Sol/BH com maior massa) */
		int attractor_idx = -1;
		double attractor_mass = 0.0;
		for (int i = 0; i < n_bodies; i++) {
			if (bodies[i].type == BHS_BODY_STAR || bodies[i].type == BHS_BODY_BLACKHOLE) {
				if (bodies[i].state.mass > attractor_mass) {
					attractor_mass = bodies[i].state.mass;
					attractor_idx = i;
				}
			}
		}
		
		if (attractor_idx >= 0) {
			const struct bhs_body *attractor = &bodies[attractor_idx];
			
			/* Cor vermelha pra linha */
			struct bhs_ui_color red = { 1.0f, 0.2f, 0.2f, 0.7f };
			
			for (int i = 0; i < n_bodies; i++) {
				/* Pula o próprio atrator */
				if (i == attractor_idx) continue;
				
				/* Se há seleção, só desenha a linha do selecionado */
				if (assets->selected_body_index >= 0 && i != assets->selected_body_index)
					continue;
				
				/* Só planetas têm linhas pro Sol */
				if (bodies[i].type != BHS_BODY_PLANET) continue;
				
				const struct bhs_body *planet = &bodies[i];
				
				/* Calcula direção planet -> attractor */
				double vx = attractor->state.pos.x - planet->state.pos.x;
				double vy = attractor->state.pos.y - planet->state.pos.y;
				double vz = attractor->state.pos.z - planet->state.pos.z;
				double dist = sqrt(vx*vx + vy*vy + vz*vz);
				
				if (dist < 0.0001) continue;
				
				/* Normaliza */
				vx /= dist; vy /= dist; vz /= dist;
				
				/* Ponto inicial: superfície do planeta (direção do atrator) */
				float start_x = (float)(planet->state.pos.x + vx * planet->state.radius);
				float start_z = (float)(planet->state.pos.z + vz * planet->state.radius);
				
				/* Ponto final: superfície do atrator (direção do planeta) */
				float end_x = (float)(attractor->state.pos.x - vx * attractor->state.radius);
				float end_z = (float)(attractor->state.pos.z - vz * attractor->state.radius);
				
				/* Calcula depth visual (pra alinhar com a malha de gravidade) */
				float depth1 = calculate_gravity_depth(start_x, start_z, bodies, n_bodies);
				float depth2 = calculate_gravity_depth(end_x, end_z, bodies, n_bodies);
				
				/* Projeta pro screen space */
				float sx1, sy1, sx2, sy2;
				bhs_project_point(cam, start_x, depth1, start_z, (float)width, (float)height, &sx1, &sy1);
				bhs_project_point(cam, end_x, depth2, end_z, (float)width, (float)height, &sx2, &sy2);
				
				/* Desenha a linha vermelha */
				bhs_ui_draw_line(ctx, sx1, sy1, sx2, sy2, red, 2.5f);
			}
		}
	}

	/* 2.6. Orbit Trails (Blue path showing planet movement history) */
	if (assets && assets->show_orbit_trail && n_bodies > 0) {
		struct bhs_ui_color trail_color = { 0.2f, 0.5f, 1.0f, 0.6f }; /* Azul semi-transparente */
		
		for (int i = 0; i < n_bodies; i++) {
			const struct bhs_body *planet = &bodies[i];
			
			/* Só planetas têm trails */
			if (planet->type != BHS_BODY_PLANET) continue;
			
			/* Mesma lógica de filtro: se há seleção, só desenha trail do selecionado */
			if (assets->selected_body_index >= 0 && i != assets->selected_body_index)
				continue;
			
			/* Precisa de pelo menos 2 pontos para desenhar linha */
			if (planet->trail_count < 2) continue;
			
			/* Desenha segmentos conectando pontos históricos */
			for (int j = 0; j < planet->trail_count - 1; j++) {
				/* Calcula índices no buffer circular */
				int start_idx = (planet->trail_head - planet->trail_count + j + BHS_MAX_TRAIL_POINTS) % BHS_MAX_TRAIL_POINTS;
				int end_idx = (start_idx + 1) % BHS_MAX_TRAIL_POINTS;
				
				float x1 = planet->trail_positions[start_idx][0];
				float z1 = planet->trail_positions[start_idx][2];
				float x2 = planet->trail_positions[end_idx][0];
				float z2 = planet->trail_positions[end_idx][2];
				
				/* Trilhas flat (Y=0) - não seguir curvatura da gravidade */
				/* Isso evita gaps causados por projeção incorreta de Y negativo */
				float depth = 0.0f;
				
				/* Projeta pro screen space */
				float sx1, sy1, sx2, sy2;
				bhs_project_point(cam, x1, depth, z1, (float)width, (float)height, &sx1, &sy1);
				bhs_project_point(cam, x2, depth, z2, (float)width, (float)height, &sx2, &sy2);
				
				/* Clipping simples: só desenha se os pontos estão dentro da tela */
				bool on_screen1 = (sx1 > -100 && sx1 < width + 100 && sy1 > -100 && sy1 < height + 100);
				bool on_screen2 = (sx2 > -100 && sx2 < width + 100 && sy2 > -100 && sy2 < height + 100);
				
				if (on_screen1 || on_screen2) {
					bhs_ui_draw_line(ctx, sx1, sy1, sx2, sy2, trail_color, 1.5f);
				}
			}
		}
	}

	/* [NEW] 2.7. Orbit Completion Markers (Marcadores Roxos de Órbita Completa) */
	if (assets && assets->orbit_markers && assets->orbit_markers->marker_count > 0) {
		/* Roxo vibrante pro marcador */
		struct bhs_ui_color purple = { 0.6f, 0.2f, 0.8f, 1.0f };
		struct bhs_ui_color purple_outline = { 0.9f, 0.6f, 1.0f, 0.9f };

		const struct bhs_orbit_marker_system *sys = assets->orbit_markers;

		/* [FIX] Para evitar a poluição de pontos roxos ("stacking"), 
		 * vamos descobrir qual o orbit_number mais alto de cada planeta.
		 */
		int latest_orbit[128]; /* MAX_BODIES */
		for(int i=0; i<128; i++) latest_orbit[i] = -1;

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

			/* [FIX] Só desenha se for a órbita mais recente do planeta */
			if (m->planet_index >= 0 && m->planet_index < 128) {
				if (m->orbit_number != latest_orbit[m->planet_index]) continue;
			}

			/* [FIX] Respeita o isolamento visual */
			if (assets->isolated_body_index >= 0 && m->planet_index != assets->isolated_body_index)
				continue;

			/* Projeta posição do marcador */
			float sx, sy;
			bhs_project_point(cam, (float)m->position.x, 0.0f, (float)m->position.z,
				      (float)width, (float)height, &sx, &sy);

			/* Só desenha se visível na tela */
			if (sx < -50 || sx > width + 50 || sy < -50 || sy > height + 50)
				continue;

			/* Desenha diamante roxo (losango) */
			float size = 10.0f;
			bhs_ui_draw_circle_fill(ctx, sx, sy, size, purple);
			
			/* Borda brilhante */
			bhs_ui_draw_circle_fill(ctx, sx, sy, size + 2.0f, (struct bhs_ui_color){0.9f, 0.6f, 1.0f, 0.3f});

			/* Número da órbita pequeno ao lado */
			char orbit_text[16];
			snprintf(orbit_text, sizeof(orbit_text), "#%d", m->orbit_number);
			bhs_ui_draw_text(ctx, orbit_text, sx + size + 3.0f, sy - 5.0f,
					 10.0f, purple_outline);
		}
	}

	for (int i = 0; i < n_bodies; i++) {
		const struct bhs_body *b = &bodies[i];

		/* [NEW] Isolamento: se ativo, pula corpos que não são o selecionado */
		if (assets && assets->isolated_body_index >= 0 && i != assets->isolated_body_index)
			continue;

		float sx, sy;

        /* Visual Mapping:
           Physics Pos (x, y) -> Visual (x, z) 
           Y is 0 in 2D physics.
        */
		float visual_x = b->state.pos.x;
		float visual_z = b->state.pos.z; /* Physics Z -> Visual Z (Matches presets.c X-Z orbit) */
		
		/* Calculate depth based on gravity well logic */
		float visual_y = calculate_gravity_depth(visual_x, visual_z, bodies, n_bodies);

		bhs_project_point(cam, visual_x, visual_y, visual_z, width, height, &sx, &sy);
		
		/* ... [Using same projection/scaling logic as before] ... */
		float dx = visual_x - cam->x;
		float dy = visual_y - cam->y; /* Including depth in distance */
		float dz = visual_z - cam->z;
		float dist = sqrtf(dx*dx + dy*dy + dz*dz);
		if (dist < 0.1f) dist = 0.1f;
		
		/* VISUAL SCALE: Uniform 80x */
		float visual_radius = b->state.radius * 80.0f;
		float s_radius = (visual_radius / dist) * cam->fov;
		if (s_radius < 2.0f) s_radius = 2.0f;

		struct bhs_ui_color color = { (float)b->color.x, (float)b->color.y, (float)b->color.z, 1.0f };
		
		/* [FIX] Aggressive Refactor: NO 2D DRAWING for Celestial Bodies */
		/* We only draw labels for planets/stars to help identification in 3D view */
		if (b->type == BHS_BODY_PLANET || b->type == BHS_BODY_STAR || b->type == BHS_BODY_MOON) {
            /* Only draw text if visible on screen */
            if (sx > 0 && sx < width && sy > 0 && sy < height) {
    			const char *label = (b->name[0]) ? b->name : "Planet";
	    		bhs_ui_draw_text(ctx, label, sx + 5, sy, 12.0f, BHS_UI_COLOR_WHITE);
            }
			/* NO 2D SPRITES - "Garantir ao supremo que o planeta tá só 3d" */
		} else {
             /* Other bodies (Asteroids, Ships?) can stay simple circles for now */
             bhs_ui_draw_circle_fill(ctx, sx, sy, s_radius, color);
		}
	}
}

