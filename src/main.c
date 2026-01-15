/**
 * @file main.c
 * @brief Ponto de entrada do Black Hole Simulator
 *
 * "Onde tudo começa. E se der segfault, onde tudo termina."
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "src/system/application.h"
#include "framework/log.h"
#include "framework/rhi/renderer.h"
#include "src/ui/screens/view_spacetime.h"
#include "src/ui/camera/camera.h"
#include "engine/assets/image_loader.h"
#include "src/debug/telemetry.h"
#include "src/simulation/planets/planet.h"
#include "engine/components/body/body.h"

/* --- Helper: Projection for Picking (Duplicated from Renderer for simplicity) --- */
static void project_point(const bhs_camera_t *c, float x, float y,
			float z, float sw, float sh, float *ox,
			float *oy)
{
	float dx = x - c->x;
	float dy = y - c->y;
	float dz = z - c->z;

	float cos_yaw = cosf(c->yaw);
	float sin_yaw = sinf(c->yaw);
	float x1 = dx * cos_yaw - dz * sin_yaw;
	float z1 = dx * sin_yaw + dz * cos_yaw;
	float y1 = dy;

	float cos_pitch = cosf(c->pitch);
	float sin_pitch = sinf(c->pitch);
	float y2 = y1 * cos_pitch - z1 * sin_pitch;
	float z2 = y1 * sin_pitch + z1 * cos_pitch;
	float x2 = x1;

	if (z2 < 0.1f)
		z2 = 0.1f;
	float factor = c->fov / z2;
	*ox = x2 * factor + sw * 0.5f;
	*oy = (sh * 0.5f) - (y2 * factor);
}

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	setbuf(stdout, NULL);

	/* 
     * REFACTORED CASCADE INIT 
     */
    bhs_app_t app = {0};
    if (!bhs_app_init(&app)) {
        return 1;
    }
    
    // Alias for convenience in loop
    bhs_ui_ctx_t ui = app.ui;
    bhs_scene_t scene = app.scene;
    bhs_gpu_texture_t bg_tex = app.bg_tex;
    bhs_gpu_texture_t sphere_tex = app.sphere_tex;

	/* 3. Inicializa Camera (Local state needed for loop) */
	bhs_camera_t cam;
	bhs_camera_init(&cam);

    BHS_LOG_INFO("Loop Start");

	/* 4. Loop Principal */
	/* Time */
	double dt = 0.016; /* 60 FPS fixo */

	/* Loop */
	/* --- Helper: Projection for Picking (Duplicated from Renderer for simplicity) --- */


	/* Loop */
	while (!bhs_ui_should_close(ui)) {
		/* UI Framework handles polling inside begin_frame or internal loop
     * mechanisms */

		/* Inicia Frame */
		if (bhs_ui_begin_frame(ui) != BHS_UI_OK) {
			continue; /* Frame perdido, vida que segue */
		}

		/* --- INTERACTION LOGIC START --- */
		int win_w, win_h;
		bhs_ui_get_size(ui, &win_w, &win_h);

		/* 1. Handle Object Deletion */
		if (app.hud_state.req_delete_body) {
			if (app.hud_state.selected_body_index != -1) {
				bhs_scene_remove_body(
					scene, app.hud_state.selected_body_index);
				app.hud_state.selected_body_index = -1;
			}
			app.hud_state.req_delete_body = false;
		}

		/* 2. Handle Object Injection */
		if (app.hud_state.req_add_body_type != -1 || app.hud_state.req_add_registry_entry) {
			/* Add slightly in front of camera */
			float spawn_dist = 20.0f;
			struct bhs_vec3 pos = {
				.x = cam.x + sinf(cam.yaw) * spawn_dist,
				.y = 0.0f,
				.z = cam.z + cosf(cam.yaw) * spawn_dist
			};
			struct bhs_vec3 vel = { 0, 0, 0 };

			struct bhs_body new_body;
			
			/* Case A: From Registry (New System) */
			if (app.hud_state.req_add_registry_entry) {
				const struct bhs_planet_desc desc = app.hud_state.req_add_registry_entry->getter();
				/* Scale down for simulation consistency if coming from raw SI getter? 
				 * presets.c does scaling. 
				 * We should probably apply the same scaling here: 
				 * 1e29 kg -> 1.0 mass
				 * 1e7 m -> 1.0 radius
				 */
				#define MAIN_MASS_SCALE (1.0 / 1e29)
				#define MAIN_RADIUS_SCALE (3.0 / 6.9634e8)
				
				new_body = bhs_body_create_from_desc(&desc, pos);
				new_body.state.mass *= MAIN_MASS_SCALE;
				new_body.state.radius *= MAIN_RADIUS_SCALE;
				
				/* Minimum simulation stability size, but allow small visual size */
				if (new_body.state.mass < 1e-10) new_body.state.mass = 1e-10;

				printf("[SPAWN] Registry: %s (M=%.2e, R=%.2f)\n", 
					desc.name, new_body.state.mass, new_body.state.radius);

				app.hud_state.req_add_registry_entry = NULL;
			} 
			/* Case B: Legacy Hardcoded (Fallback) */
			else { 
				struct bhs_vec3 col = { (float)rand() / RAND_MAX,
							(float)rand() / RAND_MAX,
							(float)rand() / RAND_MAX };
				double mass = 0.1;
				double radius = 0.5;

				if (app.hud_state.req_add_body_type == BHS_BODY_STAR) {
					mass = 2.0;
					radius = 1.0;
					col = (struct bhs_vec3){ 1.0, 0.8, 0.2 };
				} else if (app.hud_state.req_add_body_type == BHS_BODY_BLACKHOLE) {
					mass = 10.0;
					radius = 2.0;
					col = (struct bhs_vec3){ 0.0, 0.0, 0.0 };
				}
				
				new_body = bhs_body_create_planet_simple(pos, mass, radius, col);
				new_body.type = app.hud_state.req_add_body_type;
			}

			/* Auto-Orbit Logic (Shared) */
			if (new_body.type == BHS_BODY_PLANET) {
				/* Find central mass */
				int n_bodies;
				const struct bhs_body *bodies = bhs_scene_get_bodies(scene, &n_bodies);
				double central_mass = 0;
				for (int i = 0; i < n_bodies; i++) {
					if (bodies[i].type == BHS_BODY_BLACKHOLE || 
					    bodies[i].type == BHS_BODY_STAR) {
						central_mass += bodies[i].state.mass;
					}
				}

				if (central_mass > 0) {
					double r = sqrt(pos.x * pos.x + pos.z * pos.z);
					if (r > 0.1) {
						double v_orb = sqrt(central_mass / r);
						double dir_x = -pos.z / r;
						double dir_z = pos.x / r;
						vel.x = dir_x * v_orb;
						vel.z = dir_z * v_orb;
					}
				}
			}
			new_body.state.vel = vel;
			
			/* Ensure defaults if missing */
			if (new_body.name[0] == '\0') {
				if (new_body.type == BHS_BODY_PLANET) strncpy(new_body.name, "Planet", 31);
				else if (new_body.type == BHS_BODY_STAR) strncpy(new_body.name, "Star", 31);
				else if (new_body.type == BHS_BODY_BLACKHOLE) strncpy(new_body.name, "Black Hole", 31);
			}

			bhs_scene_add_body_struct(scene, new_body);

			app.hud_state.req_add_body_type = -1;
		}

		/* 3. Handle Picking (Selection) */
		if (bhs_ui_mouse_clicked(ui, 0)) { /* Left Click */
			/* Check if we clicked on UI? Simple assumption: if mouse Y < 30 (top bar) or inside panel, ignore world pick.
         * For now, just simplistic world pick.
         */
			int mx, my;
			bhs_ui_mouse_pos(ui, &mx, &my);

			/* 3.0 Check UI Block */
			if (bhs_hud_is_mouse_over(&app.hud_state, mx, my, win_w,
						  win_h)) {
				/* Click was on UI, ignore picking */
				goto skip_picking;
			}

			int n_bodies;
			const struct bhs_body *bodies =
				bhs_scene_get_bodies(scene, &n_bodies);
			int best_idx = -1;
			float best_dist = 10000.0f; /* Screen pixels */

			for (int i = 0; i < n_bodies; i++) {
				float sx, sy;
				/* Reuse visual correction logic? Simplify: just use physics pos */
				project_point(&cam, (float)bodies[i].state.pos.x,
					      (float)bodies[i].state.pos.y,
					      (float)bodies[i].state.pos.z,
					      (float)win_w, (float)win_h, &sx,
					      &sy);

				float d2 = (sx - mx) * (sx - mx) +
					   (sy - my) * (sy - my);
				float radius_sq =
					20.0f * 20.0f; /* Click tolerance */

				/* Scale tolerance by object visual size? */
				if (d2 < radius_sq && d2 < best_dist) {
					best_dist = d2;
					best_idx = i;
				}
			}

			/* Update Selection Loop */
			app.hud_state.selected_body_index = best_idx;
		}
skip_picking:

		/* 4. Update Cache for HUD */
		if (app.hud_state.selected_body_index != -1) {
			int n;
			const struct bhs_body *b =
				bhs_scene_get_bodies(scene, &n);
			if (app.hud_state.selected_body_index < n) {
				app.hud_state.selected_body_cache =
					b[app.hud_state.selected_body_index];
			} else {
				app.hud_state.selected_body_index =
					-1; /* Body sumiu */
			}
		}
		/* --- INTERACTION LOGIC END --- */

		/* Inicia Gravação de Comandos e Render Pass */
		bhs_ui_cmd_begin(ui);
		bhs_ui_begin_drawing(ui);

		/* Atualiza Física (dt fixo 60fps por enquanto) */
		bhs_scene_update(scene, dt);

		/* Atualiza Câmera (Input) */
		bhs_camera_update_view(&cam, ui, dt);

		/* Limpa tela (Preto absoluto para contraste máximo) */
		bhs_ui_clear(ui,
			     (struct bhs_ui_color){ 0.0f, 0.0f, 0.0f, 1.0f });

		/* Desenha Malha Espacial (Passamos a textura aqui) */
		/* Desenha Malha Espacial (Passamos a textura aqui) */
		bhs_view_assets_t assets = { .bg_texture = bg_tex,
					     .sphere_texture = sphere_tex,
					     .show_grid = app.hud_state.show_grid };
		bhs_view_spacetime_draw(ui, scene, &cam, win_w, win_h, &assets);

		/* Interface Adicional (HUD) */
		bhs_hud_draw(ui, &app.hud_state, win_w, win_h);

		/* Text info inferior (permanente) */
		bhs_ui_draw_text(
			ui,
			"Status: Interactive Mode (Click objects to select)",
			10, (float)win_h - 30, 16.0f, BHS_UI_COLOR_GRAY);

		/* Finaliza Frame */
		bhs_ui_end_frame(ui);

		/* Telemetry Update (every 0.5s approx, since dt=0.016, 30 frames) */
		static int frame_count = 0;
		static double total_time = 0.0;
		frame_count++;
		total_time += dt;
		if (frame_count % 30 == 0) {
			bhs_telemetry_print_scene(scene, total_time,
						  app.hud_state.show_grid);
		}
	}

	printf("Desligando simulacao...\n");

	/* 4. Cleanup */
    bhs_app_shutdown(&app);

	return 0;
}
