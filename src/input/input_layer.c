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
#include "gui/ui/lib.h"
#include "gui/log.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * HELPERS DE PROJEÇÃO (para picking)
 * ============================================================================
 */

#include "src/math/mat4.h"

/**
 * Projeta ponto 3D para coordenadas de tela
 * ATUALIZADO: Usa mesma lógica do planet_renderer.c
 */
static void project_point(const bhs_camera_t *c, float x, float y, float z,
			  float sw, float sh, float *ox, float *oy)
{
	/* Ponto já está em coordenadas RTC (relativo à câmera) */
	
	/* 1. View Matrix (igual planet_renderer.c) */
	float cy = cosf(c->yaw);
	float sy = sinf(c->yaw);
	float cp = cosf(c->pitch);
	float sp = sinf(c->pitch);
	
	/* Yaw (Y-Axis) */
	bhs_mat4_t m_yaw = bhs_mat4_identity();
	m_yaw.m[0] = cy;
	m_yaw.m[2] = sy;
	m_yaw.m[8] = -sy;
	m_yaw.m[10] = cy;
	
	/* Pitch (X-Axis) */
	bhs_mat4_t m_pitch = bhs_mat4_identity();
	m_pitch.m[5] = cp;
	m_pitch.m[6] = sp;
	m_pitch.m[9] = -sp;
	m_pitch.m[10] = cp;
	
	/* View = Pitch * Yaw */
	bhs_mat4_t mat_view = bhs_mat4_mul(m_pitch, m_yaw);
	
	/* 2. Projection Matrix */
	float focal_length = c->fov;
	if (focal_length < 1.0f) focal_length = 1.0f;
	
	float fov_y = 2.0f * atanf((sh * 0.5f) / focal_length);
	float aspect = sw / sh;
	
	bhs_mat4_t mat_proj = bhs_mat4_perspective(fov_y, aspect, 1000.0f, 1.0e14f);
	
	/* 3. ViewProj */
	bhs_mat4_t mat_vp = bhs_mat4_mul(mat_proj, mat_view);
	
	/* 4. Transformar ponto */
	struct bhs_v4 pos = { x, y, z, 1.0f };
	struct bhs_v4 clip = bhs_mat4_mul_v4(mat_vp, pos);
	
	/* 5. Perspective divide */
	if (fabsf(clip.w) < 0.001f) {
		*ox = sw * 0.5f;
		*oy = sh * 0.5f;
		return;
	}
	
	float ndc_x = clip.x / clip.w;
	float ndc_y = clip.y / clip.w;
	
	/* 6. NDC -> Screen (Vulkan: Y já está invertido na projection) */
	*ox = (ndc_x + 1.0f) * 0.5f * sw;
	*oy = (ndc_y + 1.0f) * 0.5f * sh;
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
static void handle_simulation_input(struct app_state *app, double dt)
{
	(void)dt; /* Agora não usado após remoção do grid control */
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
			
			/* Use Canonical Unit Conversion (SI -> Sim) - REMOVED for SI Scale */
			new_body.state.mass = new_body.state.mass;
			new_body.state.radius = new_body.state.radius;

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
				mass = 2.0e30; /* Solar Mass */
				radius = 7.0e8; /* Solar Radius */
				col = (struct bhs_vec3){ 1.0, 0.8, 0.2 };
			} else if (app->hud.req_add_body_type == BHS_BODY_BLACKHOLE) {
				mass = 1.0e31; /* 5 Solar Masses */
				radius = 30000.0; /* Schwarzschild Radius approx */
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
		float best_dist = 1e9f;

		for (int i = 0; i < n_bodies; i++) {
			const struct bhs_body *b = &bodies[i];
			
			/* Só pega planetas, estrelas e luas (como no renderer) */
			if (b->type != BHS_BODY_PLANET && 
			    b->type != BHS_BODY_STAR && 
			    b->type != BHS_BODY_MOON) continue;

			/* Calcula visual_y igual planet_renderer.c */
			float visual_y = 0.0f;
			float visual_x = (float)b->state.pos.x;
			float visual_z = (float)b->state.pos.z;
			
			/* Gravity depth (igual calculate_gravity_depth) */
			float potential = 0.0f;
			for (int j = 0; j < n_bodies; j++) {
				float dx = visual_x - (float)bodies[j].state.pos.x;
				float dz = visual_z - (float)bodies[j].state.pos.z;
				float dist = sqrtf(dx*dx + dz*dz + 0.1f);
				
				double eff_mass = bodies[j].state.mass;
				if (bodies[j].type == BHS_BODY_PLANET) {
					eff_mass *= 5000.0;
				}
				potential -= eff_mass / dist;
			}
			visual_y = potential * 5.0f;
			if (visual_y < -50.0f) visual_y = -50.0f;

			/* Coordenadas RTC (Relative To Camera) - igual planet_renderer */
			float rtc_x = visual_x - (float)app->camera.x;
			float rtc_y = visual_y - (float)app->camera.y;
			float rtc_z = visual_z - (float)app->camera.z;

			/* Projeta usando as coordenadas RTC */
			float sx, sy;
			project_point(&app->camera, rtc_x, rtc_y, rtc_z,
				      (float)win_w, (float)win_h, &sx, &sy);

			/* Raio de picking proporcional ao tamanho visual */
			float visual_radius = (float)b->state.radius * 80.0f;
			float dist_to_cam = sqrtf(rtc_x*rtc_x + rtc_y*rtc_y + rtc_z*rtc_z);
			float pick_radius = 30.0f; /* Base */
			if (dist_to_cam > 0.1f) {
				/* Projeta o raio visual para pixels (aproximado) */
				float projected_r = (visual_radius / dist_to_cam) * app->camera.fov;
				if (projected_r > pick_radius) pick_radius = projected_r;
			}
			if (pick_radius < 15.0f) pick_radius = 15.0f;
			if (pick_radius > 200.0f) pick_radius = 200.0f;

			float d2 = (sx - mx) * (sx - mx) + (sy - my) * (sy - my);
			float radius_sq = pick_radius * pick_radius;

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
	handle_simulation_input(app, dt);
	handle_camera_input(app, dt);
	handle_object_interaction(app);
}
