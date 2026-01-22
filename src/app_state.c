/**
 * @file app_state.c
 * @brief Implementação do Ciclo de Vida da Aplicação
 *
 * "O código que faz o circo funcionar sem que a plateia veja os palhaços."
 *
 * Este arquivo implementa o boot, loop principal e shutdown da aplicação.
 * Se você está aqui, provavelmente algo quebrou. Boa sorte.
 */

#include "app_state.h"
#include "simulation/scenario_mgr.h"
#include "simulation/systems/systems.h" // [NEW] Logic Systems

#include <math.h> /* [NEW] for powf/fabs */
#include "engine/assets/image_loader.h"
#include "gui/log.h"
#include "gui/rhi/rhi.h"
#include "src/simulation/data/planet.h" /* Registry is here */

#include "src/debug/telemetry.h"
#include "src/input/input_layer.h"
#include "src/ui/render/blackhole_pass.h" /* [NEW] */
#include "src/ui/screens/view_spacetime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* 
 * Timestep fixo pra física - 1 HORA por passo (3600 segundos)
 * Para órbitas planetárias, precisamos simular até 365 dias por minuto real.
 * 365 dias/min = 525,600 segundos simulados por segundo real.
 * Com dt=3600s, precisamos de apenas 146 passos/segundo = ~2.4 passos/frame.
 * Isso é trivial para qualquer CPU.
 */
#define PHYSICS_DT 3600.0
#define MAX_FRAME_TIME 0.25 /* Evita death spiral */

/* ============================================================================
 * HELPERS
 * ============================================================================
 */

static double get_time_seconds(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

/* ============================================================================
 * INICIALIZAÇÃO
 * ============================================================================
 */

bool app_init(struct app_state *app, const char *title, int width, int height)
{
	if (!app)
		return false;

	/* 0. Logging (PRIMEIRO - antes de qualquer log) */
	bhs_log_init();
	bhs_log_set_level(BHS_LOG_LEVEL_DEBUG); /* [DEBUG] Force Debug Level */
	BHS_LOG_INFO("=== BlackHole TensorRay - Inicializando ===");

	/* 1. Scene / Engine Memory */
	BHS_LOG_INFO("Alocando memória da Engine...");
	app->scene = bhs_scene_create();
	if (!app->scene) {
		BHS_LOG_FATAL("Falha ao criar scene!");
		goto err_scene;
	}

	/* 2. gui/UI (Window + Vulkan) */
	BHS_LOG_INFO("Inicializando gui/UI...");
	struct bhs_ui_config config = { .title = title ? title
						       : "BlackHole TensorRay",
					.width = width > 0 ? width : 1280,
					.height = height > 0 ? height : 720,
					.resizable = true,
					.vsync = true,
					.debug = true };

	int ret = bhs_ui_create(&config, &app->ui);
	if (ret != BHS_UI_OK) {
		BHS_LOG_FATAL("Falha ao criar UI: %d", ret);
		goto err_ui;
	}

	/* 3. HUD State */
	bhs_hud_init(&app->hud);

	/* 4. Assets - Skybox */
	BHS_LOG_INFO("Carregando assets...");
	bhs_image_t bg_img = bhs_image_load("src/assets/textures/space_bg.png");
	if (bg_img.data) {
		struct bhs_gpu_texture_config tex_conf = {
			.width = bg_img.width,
			.height = bg_img.height,
			.depth = 1,
			.mip_levels = 1,
			.array_layers = 1,
			.format = BHS_FORMAT_RGBA8_SRGB,
			.usage = BHS_TEXTURE_SAMPLED | BHS_TEXTURE_TRANSFER_DST,
			.label = "Skybox"
		};
		bhs_gpu_device_t dev = bhs_ui_get_gpu_device(app->ui);
		if (bhs_gpu_texture_create(dev, &tex_conf, &app->bg_tex) ==
		    BHS_GPU_OK) {
			bhs_gpu_texture_upload(app->bg_tex, 0, 0, bg_img.data,
					       bg_img.width * bg_img.height *
						       4);
		}
		bhs_image_free(bg_img);
	} else {
		BHS_LOG_WARN("Skybox não encontrado - usando fundo preto");
	}

	/* 4.1. Sphere Impostor */
	{
		int size = 64;
		bhs_image_t sphere_img = bhs_image_gen_sphere(size);
		if (sphere_img.data) {
			struct bhs_gpu_texture_config conf = {
				.width = size,
				.height = size,
				.depth = 1,
				.mip_levels = 1,
				.array_layers = 1,
				.format = BHS_FORMAT_RGBA8_UNORM,
				.usage = BHS_TEXTURE_SAMPLED |
					 BHS_TEXTURE_TRANSFER_DST,
				.label = "Sphere Impostor"
			};
			bhs_gpu_device_t dev = bhs_ui_get_gpu_device(app->ui);
			bhs_gpu_texture_create(dev, &conf, &app->sphere_tex);
			if (app->sphere_tex) {
				bhs_gpu_texture_upload(app->sphere_tex, 0, 0,
						       sphere_img.data,
						       size * size * 4);
			}
			bhs_image_free(sphere_img);
		}
	}

	/* 4.1.5. [NEW] Procedural Planet Textures (The Awakening) */
	{
		BHS_LOG_INFO("Gerando texturas procedurais dos planetas...");
		bhs_gpu_device_t dev = bhs_ui_get_gpu_device(app->ui);

		const struct bhs_planet_registry_entry *entry =
			bhs_planet_registry_get_head();
		int count = 0;

		while (entry && count < 32) {
			if (entry->getter) {
				struct bhs_planet_desc desc = entry->getter();
				BHS_LOG_INFO("  > Gerando surface: %s...",
					     desc.name);

				/* Generate High-Res Texture (1024x512) */
				/* Note: Large textures increase boot time. Keep modest for now. */
				bhs_image_t img = bhs_image_gen_planet_texture(
					&desc, 1024,
					512); // Aumentar resolução aqui

				if (img.data) {
					struct bhs_gpu_texture_config conf = {
						.width = img.width,
						.height = img.height,
						.depth = 1,
						.mip_levels = 1,
						.array_layers = 1,
						.format =
							BHS_FORMAT_RGBA8_UNORM, /* Albedo */
						.usage =
							BHS_TEXTURE_SAMPLED |
							BHS_TEXTURE_TRANSFER_DST,
						.label = desc.name
					};

					bhs_gpu_texture_t tex = NULL;
					if (bhs_gpu_texture_create(dev, &conf,
								   &tex) ==
					    BHS_GPU_OK) {
						bhs_gpu_texture_upload(
							tex, 0, 0, img.data,
							img.width * img.height *
								4);

						/* Cache it */
						strncpy(app->tex_cache[count]
								.name,
							desc.name, 31);
						app->tex_cache[count].tex = tex;
						count++;
					}

					bhs_image_free(img);
				}
			}
			entry = entry->next;
		}
		app->tex_cache_count = count;
		BHS_LOG_INFO("Geradas %d texturas de planetas.", count);
	}

	/* 4.2. Black Hole Pass (Init) */
	{
		bhs_gpu_device_t dev = bhs_ui_get_gpu_device(app->ui);
		bhs_blackhole_pass_config_t bh_conf = { .width = width,
							.height = height };
		app->bh_pass = bhs_blackhole_pass_create(dev, &bh_conf);
		if (!app->bh_pass) {
			BHS_LOG_WARN("Compute Pass falhou ao iniciar - Shader "
				     "faltando?");
		}
	}

	/* 4.3. [NEW] Planet 3D Pass */
	if (bhs_planet_pass_create(app->ui, &app->planet_pass) != 0) {
		BHS_LOG_ERROR("Falha ao inicializar renderer de planetas.");
	}

	/* 5. Camera (valores padrão) */
	bhs_camera_init(&app->camera);

	/* 6. Simulation defaults */
	app->sim_status = APP_SIM_RUNNING;
	app->time_scale = 1.0;
	app->accumulated_time = 0.0;
	app->scenario = APP_SCENARIO_NONE;
	app->should_quit = false;

	/* 6.1. [NEW] Inicializa sistema de marcadores de órbita */
	bhs_orbit_markers_init(&app->orbit_markers);

	/* 7. Timing */
	app->last_frame_time = get_time_seconds();
	app->frame_count = 0;
	app->phys_ms = 0.0;
	app->render_ms = 0.0;

	BHS_LOG_INFO("Inicialização completa. Sistemas online.");
	return true;

err_ui:
	bhs_scene_destroy(app->scene);
	app->scene = NULL;
err_scene:
	bhs_log_shutdown();
	return false;
}

/* ============================================================================
 * LOOP PRINCIPAL
 * ============================================================================
 */

void app_run(struct app_state *app)
{
	if (!app || !app->ui || !app->scene) {
		BHS_LOG_FATAL("app_run chamado com estado inválido");
		return;
	}

	BHS_LOG_INFO("Entrando no loop principal...");
	double accumulator = 0.0;

	while (!app->should_quit && !bhs_ui_should_close(app->ui)) {
		/* Timing */
		double current_time = get_time_seconds();
		double frame_time = current_time - app->last_frame_time;
		app->last_frame_time = current_time;

		/* Evitar death spiral */
		if (frame_time > MAX_FRAME_TIME)
			frame_time = MAX_FRAME_TIME;

		/* Sync Time Scale from HUD PRIMEIRO - antes de acumular tempo */
		/* Formula: dias/min = 0.1 * 3650^val */
		/* 1 dia = 86400 segundos, 1 minuto real = 60 segundos */
		/* timescale = dias/min * 86400 / 60 = dias/min * 1440 */
		{
			float days_per_min =
				0.1f * powf(3650.0f, app->hud.time_scale_val);
			float target_timescale = days_per_min * 1440.0f;
			app_set_time_scale(app, (double)target_timescale);
		}

		/* Acumular tempo para fixed timestep - MULTIPLICADO pelo time_scale! */
		accumulator += frame_time * app->time_scale;

		/* Begin frame */
		if (bhs_ui_begin_frame(app->ui) != BHS_UI_OK)
			continue; /* Frame perdido, vida que segue */

		/* Processar input (atualiza câmera e estado) */
		input_process_frame(app, frame_time);

		/* Inicia gravação de comandos */
		bhs_ui_cmd_begin(app->ui);
		bhs_ui_begin_drawing(app->ui);

/* Fixed timestep para física */
/* Limita número de passos por frame para evitar death spiral */
#define MAX_PHYSICS_STEPS_PER_FRAME 1000
		int physics_steps = 0;

		double t0 = get_time_seconds();
		while (accumulator >= PHYSICS_DT &&
		       app->sim_status == APP_SIM_RUNNING &&
		       physics_steps < MAX_PHYSICS_STEPS_PER_FRAME) {
			/* [NEW] ECS Systems Update (Leapfrog + 1PN) */
			bhs_world_handle world =
				bhs_scene_get_world(app->scene);

			/* High-Fidelity Physics Integration */
			physics_system_update(world, PHYSICS_DT);

			/* 3. Engine Update (Collision, Transfrom hierarchy, Spacetime sync) */
			bhs_scene_update(app->scene, PHYSICS_DT);

			/* 4. Gameplay/Celestial Update (Rotation, Events) */
			bhs_celestial_system_update(app->scene, PHYSICS_DT);

			/* [NEW] Orbit Trail Sampling (every 10 physics frames) */
			static int trail_sample_counter = 0;
			trail_sample_counter++;
			if (app->hud.show_orbit_trail &&
			    (trail_sample_counter % 10 == 0)) {
				int count = 0;
				struct bhs_body *bodies =
					(struct bhs_body *)bhs_scene_get_bodies(
						app->scene, &count);
				for (int i = 0; i < count; i++) {
					if (bodies[i].type != BHS_BODY_PLANET)
						continue;

					/* Adiciona posição atual ao buffer circular */
					int idx = bodies[i].trail_head;
					bodies[i].trail_positions[idx][0] =
						(float)bodies[i].state.pos.x;
					bodies[i].trail_positions[idx][1] =
						(float)bodies[i].state.pos.y;
					bodies[i].trail_positions[idx][2] =
						(float)bodies[i].state.pos.z;

					bodies[i].trail_head =
						(idx + 1) %
						BHS_MAX_TRAIL_POINTS;
					if (bodies[i].trail_count <
					    BHS_MAX_TRAIL_POINTS) {
						bodies[i].trail_count++;
					}
				}
			}

			/* [NEW] Atualiza sistema de marcadores de órbita */
			{
				int count = 0;
				const struct bhs_body *bodies =
					bhs_scene_get_bodies(app->scene, &count);
				bhs_orbit_markers_update(&app->orbit_markers, bodies,
							 count, app->accumulated_time);
			}

			accumulator -= PHYSICS_DT;
			app->accumulated_time += PHYSICS_DT;
			physics_steps++;
		}

		/* NOTA: Sync do time_scale foi movido para antes do acumulador */

		/* [NEW] Object Inspector: Calculate Strongest Attractor */
		if (app->hud.selected_body_index != -1) {
			int count = 0;
			const struct bhs_body *bodies =
				bhs_scene_get_bodies(app->scene, &count);

			// Validate index
			if (app->hud.selected_body_index < count) {
				const struct bhs_body *me =
					&bodies[app->hud.selected_body_index];

				double max_force = -1.0;
				int best_idx = -1;
				double best_dist = 0.0;

				for (int i = 0; i < count; i++) {
					if (i == app->hud.selected_body_index)
						continue; // Don't attract yourself
					if (!bodies[i].is_alive)
						continue;

					// Only Stars and Black Holes are "Astros" for this check
					if (bodies[i].type != BHS_BODY_STAR &&
					    bodies[i].type !=
						    BHS_BODY_BLACKHOLE)
						continue;

					double dx = bodies[i].state.pos.x -
						    me->state.pos.x;
					double dy = bodies[i].state.pos.y -
						    me->state.pos.y;
					double dz = bodies[i].state.pos.z -
						    me->state.pos.z;
					double dist_sq =
						dx * dx + dy * dy + dz * dz;

					if (dist_sq < 0.0001)
						continue; // Avoid singularity

					// Force is proportional to Mass / Dist^2 (ignoring G and my mass as constants for comparison)
					double force =
						bodies[i].state.mass / dist_sq;

					if (force > max_force) {
						max_force = force;
						best_idx = i;
						best_dist = sqrt(dist_sq);
					}
				}

				if (best_idx != -1) {
					snprintf(app->hud.attractor_name, 64,
						 "%s", bodies[best_idx].name);

					/* Surface-to-Surface Distance (Wall-to-Wall) */
					double r_attractor =
						bodies[best_idx].state.radius;
					double r_self = me->state.radius;
					double surf_dist = best_dist -
							   r_attractor - r_self;

					app->hud.attractor_dist = surf_dist;
				} else {
					app->hud.attractor_name[0] = '\0';
				}
			}
		}

		/* Rendering */
		t0 = get_time_seconds();

		int win_w, win_h;
		bhs_ui_get_size(app->ui, &win_w, &win_h);

		/* Limpa tela - preto absoluto */
		bhs_ui_clear(app->ui,
			     (struct bhs_ui_color){ 0.0f, 0.0f, 0.0f, 1.0f });

		/* [NEW] Dispatch Compute Pass */
		bhs_gpu_texture_t bh_tex = NULL;
		if (app->bh_pass &&
		    app->scenario == APP_SCENARIO_KERR_BLACKHOLE) {
			bhs_blackhole_pass_resize(app->bh_pass, win_w, win_h);
			bhs_gpu_cmd_buffer_t cmd =
				bhs_ui_get_current_cmd(app->ui);
			bhs_blackhole_pass_dispatch(app->bh_pass, cmd,
						    app->scene, &app->camera);
			bh_tex = bhs_blackhole_pass_get_output(app->bh_pass);
		}

		/* Desenha cena (Updated with Visual Mode) */
		bhs_view_assets_t assets = {
			.bg_texture = app->bg_tex,
			.sphere_texture = app->sphere_tex,
			.bh_texture = bh_tex,
			.tex_cache = (const struct bhs_planet_tex_entry *)
					     app->tex_cache,
			.tex_cache_count = app->tex_cache_count,
			.render_3d_active = (app->planet_pass != NULL),
			/* Gravity Line: passa o toggle e o índice do body selecionado */
			.show_gravity_line = app->hud.show_gravity_line,
			.selected_body_index = app->hud.selected_body_index,
			/* Orbit Trail */
			.show_orbit_trail = app->hud.show_orbit_trail,
			/* [NEW] Isolated View - propaga o índice se isolamento ativo */
			.isolated_body_index = app->hud.isolate_view ? app->hud.selected_body_index : -1,
			/* [NEW] Sistema de marcadores de órbita */
			.orbit_markers = &app->orbit_markers
		};
		bhs_view_spacetime_draw(app->ui, app->scene, &app->camera,
					win_w, win_h, &assets,
					app->hud.visual_mode, app->planet_pass);

		/* HUD */
		app->hud.sim_time_seconds = app->accumulated_time; /* Sync J2000 time */
		app->hud.orbit_markers_ptr = &app->orbit_markers;   /* [NEW] Passa marcadores */
		bhs_hud_draw(app->ui, &app->hud, win_w, win_h);

		/* Status bar */
		const char *status = app->sim_status == APP_SIM_PAUSED
					     ? "PAUSED"
					     : "Running";
		char status_buf[128];
		snprintf(status_buf, sizeof(status_buf),
			 "Status: %s | Time Scale: %.1fx | S=Save L=Load "
			 "Space=Pause",
			 status, app->time_scale);
		bhs_ui_draw_text(app->ui, status_buf, 10, (float)win_h - 30,
				 16.0f, BHS_UI_COLOR_GRAY);

		app->render_ms = (get_time_seconds() - t0) * 1000.0;

		/* End frame */
		bhs_ui_end_frame(app->ui);

		/* Telemetria periódica */
		app->frame_count++;
		if (app->frame_count % 30 == 0) {
			bhs_telemetry_print_scene(app->scene,
						  app->accumulated_time,
						  app->phys_ms, app->render_ms);
		}

		/* Log orbits periodically for analysis (Scrollable history) */
		if (app->frame_count % 60 == 0) {
			bhs_telemetry_log_orbits(app->scene,
						 app->accumulated_time);
		}
	}

	BHS_LOG_INFO("Saindo do loop principal...");
}

/* ============================================================================
 * SHUTDOWN
 * ============================================================================
 */

void app_shutdown(struct app_state *app)
{
	if (!app)
		return;

	BHS_LOG_INFO("Desligando aplicação...");

	/* Ordem inversa da inicialização */
	if (app->bg_tex)
		bhs_gpu_texture_destroy(app->bg_tex);
	if (app->sphere_tex)
		bhs_gpu_texture_destroy(app->sphere_tex);

	/* Destroy cached textures */
	for (int i = 0; i < app->tex_cache_count; i++) {
		if (app->tex_cache[i].tex)
			bhs_gpu_texture_destroy(app->tex_cache[i].tex);
	}

	if (app->bh_pass)
		bhs_blackhole_pass_destroy(app->bh_pass);
	if (app->planet_pass)
		bhs_planet_pass_destroy(app->planet_pass);
	if (app->ui)
		bhs_ui_destroy(app->ui);
	if (app->scene)
		bhs_scene_destroy(app->scene);

	bhs_log_shutdown();

	BHS_LOG_INFO("Shutdown completo. Até a próxima.");
}
