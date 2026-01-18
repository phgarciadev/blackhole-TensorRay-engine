/**
 * @file input_layer.c
 * @brief Implementação da Camada de Input
 *
 * "Se o input não funcionar, é bug do hardware, não do código."
 * (mentira, é sempre bug do código)
 *
 * Mapeamentos de teclas:
 *   WASD   - Movimento da câmera (frente/trás/strafe)
 *   QE     - Subir/Descer
 *   Mouse  - Rotação (click + drag)
 *   Space  - Toggle pause
 *   S      - QuickSave
 *   L      - QuickLoad
 *   ESC    - Quit (ou menu futuro)
 *   +/-    - Ajusta time scale
 *   Delete - Remove corpo selecionado
 */

#include "input_layer.h"
#include "src/app_state.h"
#include "src/ui/camera/camera_controller.h"

#include "engine/ecs/ecs.h"
#include "engine/scene/scene.h"
#include "math/units.h"
#include "gui-framework/ui/lib.h"
#include "gui-framework/log.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * HELPERS DE PROJEÇÃO (para picking)
 * ============================================================================
 */

/**
 * Projeta ponto 3D para coordenadas de tela
 * (Código duplicado do antigo main.c - poderia ir pra um módulo de math)
 */
static void project_point(const bhs_camera_t *c, float x, float y, float z,
			  float sw, float sh, float *ox, float *oy)
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

/* ============================================================================
 * HANDLERS ESPECÍFICOS
 * ============================================================================
 */

/**
 * Processa inputs de câmera (WASD + mouse)
 */
static void handle_camera_input(struct app_state *app, double dt)
{
	/* Delega pro camera_controller existente */
	bhs_camera_controller_update(&app->camera, app->ui, dt);
}

/**
 * Processa controles de simulação (pause, time scale)
 */
static void handle_simulation_input(struct app_state *app)
{
	/* Toggle pause com Space */
	if (bhs_ui_key_pressed(app->ui, BHS_KEY_SPACE)) {
		app_toggle_pause(app);
		BHS_LOG_INFO("Simulação %s",
			     app->sim_status == APP_SIM_PAUSED 
			     ? "PAUSADA" : "RODANDO");
	}

	/*
	 * Time scale com teclas numéricas
	 * Nota: BHS_KEY_PLUS e BHS_KEY_MINUS não existem no lib.h
	 * Usando 1-5 como atalhos de velocidade
	 */
	if (bhs_ui_key_pressed(app->ui, BHS_KEY_1))
		app_set_time_scale(app, 0.25);
	if (bhs_ui_key_pressed(app->ui, BHS_KEY_2))
		app_set_time_scale(app, 0.5);
	if (bhs_ui_key_pressed(app->ui, BHS_KEY_3))
		app_set_time_scale(app, 1.0);
	if (bhs_ui_key_pressed(app->ui, BHS_KEY_4))
		app_set_time_scale(app, 2.0);
	if (bhs_ui_key_pressed(app->ui, BHS_KEY_5))
		app_set_time_scale(app, 4.0);
}

/**
 * Processa ações globais (save, load, quit)
 */
static void handle_global_input(struct app_state *app)
{
	/* QuickSave */
	if (bhs_ui_key_pressed(app->ui, BHS_KEY_S) &&
	    !bhs_ui_key_down(app->ui, BHS_KEY_W)) { /* Só S, não WS juntos */
		BHS_LOG_INFO("Salvando mundo...");
		bhs_ecs_save_world(bhs_scene_get_world(app->scene),
				   "saves/quicksave.bhs");
	}

	/* QuickLoad */
	if (bhs_ui_key_pressed(app->ui, BHS_KEY_L)) {
		BHS_LOG_INFO("Carregando mundo...");
		bhs_ecs_load_world(bhs_scene_get_world(app->scene),
				   "saves/quicksave.bhs");
	}

	/* Quit com ESC */
	if (bhs_ui_key_pressed(app->ui, BHS_KEY_ESCAPE))
		app->should_quit = true;
}

/**
 * Processa interação com objetos (seleção e deleção)
 */
static void handle_object_interaction(struct app_state *app)
{
	int win_w, win_h;
	bhs_ui_get_size(app->ui, &win_w, &win_h);

	/* Deleção de corpo selecionado */
	if (app->hud.req_delete_body) {
		if (app->hud.selected_body_index != -1) {
			bhs_scene_remove_body(app->scene,
					      app->hud.selected_body_index);
			app->hud.selected_body_index = -1;
		}
		app->hud.req_delete_body = false;
	}

	/* Injeção de novo corpo (via HUD) */
	if (app->hud.req_add_body_type != -1 || app->hud.req_add_registry_entry) {
		float spawn_dist = 20.0f;
		struct bhs_vec3 pos = {
			.x = app->camera.x + sinf(app->camera.yaw) * spawn_dist,
			.y = 0.0f,
			.z = app->camera.z + cosf(app->camera.yaw) * spawn_dist
		};

		struct bhs_body new_body = {0};

		/* Caso A: Do Registry */
		if (app->hud.req_add_registry_entry) {
			const struct bhs_planet_desc desc = 
				app->hud.req_add_registry_entry->getter();

			new_body = bhs_body_create_from_desc(&desc, pos);
			
			/* Use Canonical Unit Conversion (SI -> Sim) */
			new_body.state.mass = BHS_KG_TO_SIM(new_body.state.mass);
			new_body.state.radius = BHS_RADIUS_TO_SIM(new_body.state.radius);

			if (new_body.state.mass < 1e-10)
				new_body.state.mass = 1e-10;

			app->hud.req_add_registry_entry = NULL;
		}
		/* Caso B: Tipo hardcoded */
		else {
			struct bhs_vec3 col = { 
				(float)rand() / (float)RAND_MAX,
				(float)rand() / (float)RAND_MAX,
				(float)rand() / (float)RAND_MAX 
			};
			double mass = 0.1;
			double radius = 0.5;

			if (app->hud.req_add_body_type == BHS_BODY_STAR) {
				mass = 2.0;
				radius = 1.0;
				col = (struct bhs_vec3){ 1.0, 0.8, 0.2 };
			} else if (app->hud.req_add_body_type == BHS_BODY_BLACKHOLE) {
				mass = 10.0;
				radius = 2.0;
				col = (struct bhs_vec3){ 0.0, 0.0, 0.0 };
			}

			new_body = bhs_body_create_planet_simple(pos, mass, radius, col);
			new_body.type = app->hud.req_add_body_type;
		}

		/* Auto-orbit para planetas */
		if (new_body.type == BHS_BODY_PLANET) {
			int n_bodies;
			const struct bhs_body *bodies = 
				bhs_scene_get_bodies(app->scene, &n_bodies);
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
					new_body.state.vel.x = -pos.z / r * v_orb;
					new_body.state.vel.z = pos.x / r * v_orb;
				}
			}
		}

		/* Nome default */
		if (new_body.name[0] == '\0') {
			if (new_body.type == BHS_BODY_PLANET)
				strncpy(new_body.name, "Planeta", 31);
			else if (new_body.type == BHS_BODY_STAR)
				strncpy(new_body.name, "Estrela", 31);
			else if (new_body.type == BHS_BODY_BLACKHOLE)
				strncpy(new_body.name, "Black Hole", 31);
		}

		bhs_scene_add_body_struct(app->scene, new_body);
		app->hud.req_add_body_type = -1;
	}

	/* Picking com click */
	if (bhs_ui_mouse_clicked(app->ui, 0)) {
		int mx, my;
		bhs_ui_mouse_pos(app->ui, &mx, &my);

		/* Verifica se click foi na HUD */
		if (bhs_hud_is_mouse_over(&app->hud, mx, my, win_w, win_h))
			return; /* Ignora picking */

		int n_bodies;
		const struct bhs_body *bodies = 
			bhs_scene_get_bodies(app->scene, &n_bodies);

		int best_idx = -1;
		float best_dist = 10000.0f;

		for (int i = 0; i < n_bodies; i++) {
			float sx, sy;
            
            float visual_x = (float)bodies[i].state.pos.x;
            float visual_z = (float)bodies[i].state.pos.z;
            float visual_y = (float)bodies[i].state.pos.y;

            if (bodies[i].type == BHS_BODY_PLANET) {
                 /* Calculate depth based on gravity well (Doppler Logic) */
                 float potential = 0.0f;
                 for (int j = 0; j < n_bodies; j++) {
                     if (i == j) continue; 
                     float dx = visual_x - bodies[j].state.pos.x;
                     float dz = visual_z - bodies[j].state.pos.z; 
                     float r = sqrtf(dx*dx + dz*dz + 0.1f);
                     potential -= bodies[j].state.mass / r;
                 }
                 visual_y = potential * 5.0f; 
                 if (visual_y < -50.0f) visual_y = -50.0f;
            }

			project_point(&app->camera,
				      visual_x,
				      visual_y,
				      visual_z,
				      (float)win_w, (float)win_h,
				      &sx, &sy);

			float d2 = (sx - mx) * (sx - mx) + (sy - my) * (sy - my);
			float radius_sq = 20.0f * 20.0f;

			if (d2 < radius_sq && d2 < best_dist) {
				best_dist = d2;
				best_idx = i;
			}
		}

		app->hud.selected_body_index = best_idx;
	}

	/* Atualiza cache do corpo selecionado */
	if (app->hud.selected_body_index != -1) {
		int n;
		const struct bhs_body *b = bhs_scene_get_bodies(app->scene, &n);
		if (app->hud.selected_body_index < n) {
			app->hud.selected_body_cache = 
				b[app->hud.selected_body_index];
		} else {
			app->hud.selected_body_index = -1;
		}
	}
}

/* ============================================================================
 * API PÚBLICA
 * ============================================================================
 */

void input_process_frame(struct app_state *app, double dt)
{
	if (!app || !app->ui)
		return;

	handle_global_input(app);
	handle_simulation_input(app);
	handle_camera_input(app, dt);
	handle_object_interaction(app);
}
