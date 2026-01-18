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

#include "gui-framework/log.h"
#include "gui-framework/rhi/renderer.h"
#include "engine/assets/image_loader.h"
#include "src/simulation/data/planet.h" /* Registry is here */

#include "src/input/input_layer.h"
#include "src/ui/screens/view_spacetime.h"
#include "src/ui/render/blackhole_pass.h" /* [NEW] */
#include "src/debug/telemetry.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

/* ============================================================================
 * CONSTANTES
 * ============================================================================
 */

/* Timestep fixo pra física - 60 FPS */
#define PHYSICS_DT	0.016666666666666666
#define MAX_FRAME_TIME	0.25	/* Evita death spiral */

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
	bhs_log_set_level(BHS_LOG_LEVEL_INFO);
	BHS_LOG_INFO("=== BlackHole TensorRay - Inicializando ===");

	/* 1. Scene / Engine Memory */
	BHS_LOG_INFO("Alocando memória da Engine...");
	app->scene = bhs_scene_create();
	if (!app->scene) {
		BHS_LOG_FATAL("Falha ao criar scene!");
		goto err_scene;
	}

	/* 2. gui-framework/UI (Window + Vulkan) */
	BHS_LOG_INFO("Inicializando gui-framework/UI...");
	struct bhs_ui_config config = {
		.title = title ? title : "BlackHole TensorRay",
		.width = width > 0 ? width : 1280,
		.height = height > 0 ? height : 720,
		.resizable = true,
		.vsync = true,
		.debug = true
	};

	int ret = bhs_ui_create(&config, &app->ui);
	if (ret != BHS_UI_OK) {
		BHS_LOG_FATAL("Falha ao criar UI: %d", ret);
		goto err_ui;
	}

	/* 3. HUD State */
	bhs_hud_init(&app->hud);

	/* 4. Assets - Skybox */
	BHS_LOG_INFO("Carregando assets...");
	bhs_image_t bg_img = bhs_image_load("assets/textures/space_bg.png");
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
		if (bhs_gpu_texture_create(dev, &tex_conf, &app->bg_tex) == BHS_GPU_OK) {
			bhs_gpu_texture_upload(app->bg_tex, 0, 0, bg_img.data,
					       bg_img.width * bg_img.height * 4);
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
				.usage = BHS_TEXTURE_SAMPLED | BHS_TEXTURE_TRANSFER_DST,
				.label = "Sphere Impostor"
			};
			bhs_gpu_device_t dev = bhs_ui_get_gpu_device(app->ui);
			bhs_gpu_texture_create(dev, &conf, &app->sphere_tex);
			if (app->sphere_tex) {
				bhs_gpu_texture_upload(app->sphere_tex, 0, 0,
						       sphere_img.data, size * size * 4);
			}
			bhs_image_free(sphere_img);
		}
	}

	/* 4.1.5. [NEW] Procedural Planet Textures (The Awakening) */
	{
		BHS_LOG_INFO("Gerando texturas procedurais dos planetas...");
		bhs_gpu_device_t dev = bhs_ui_get_gpu_device(app->ui);
		
		const struct bhs_planet_registry_entry *entry = bhs_planet_registry_get_head();
		int count = 0;
		
		while (entry && count < 32) {
			if (entry->getter) {
				struct bhs_planet_desc desc = entry->getter();
				BHS_LOG_INFO("  > Gerando surface: %s...", desc.name);
				
				/* Generate High-Res Texture (1024x512) */
				/* Note: Large textures increase boot time. Keep modest for now. */
				bhs_image_t img = bhs_image_gen_planet_texture(&desc, 1024, 512); // Aumentar resolução aqui
				
				if (img.data) {
					struct bhs_gpu_texture_config conf = {
						.width = img.width,
						.height = img.height,
						.depth = 1,
						.mip_levels = 1,
						.array_layers = 1,
						.format = BHS_FORMAT_RGBA8_UNORM, /* Albedo */
						.usage = BHS_TEXTURE_SAMPLED | BHS_TEXTURE_TRANSFER_DST,
						.label = desc.name
					};
					
					bhs_gpu_texture_t tex = NULL;
					if (bhs_gpu_texture_create(dev, &conf, &tex) == BHS_GPU_OK) {
						bhs_gpu_texture_upload(tex, 0, 0, img.data, img.width * img.height * 4);
						
						/* Cache it */
						strncpy(app->tex_cache[count].name, desc.name, 31);
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
		bhs_blackhole_pass_config_t bh_conf = {
			.width = width,
			.height = height
		};
		app->bh_pass = bhs_blackhole_pass_create(dev, &bh_conf);
		if (!app->bh_pass) {
			BHS_LOG_WARN("Compute Pass falhou ao iniciar - Shader faltando?");
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

		/* Acumular tempo para fixed timestep */
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
		double t0 = get_time_seconds();
		while (accumulator >= PHYSICS_DT && app->sim_status == APP_SIM_RUNNING) {
			/* [NEW] ECS Systems Update (Leapfrog + 1PN) */
			bhs_world_handle world = bhs_scene_get_world(app->scene);
			
			/* High-Fidelity Physics Integration */
			physics_system_update(world, PHYSICS_DT);

			/* 3. Engine Update (Collision, Transfrom hierarchy, Spacetime sync) */
			bhs_scene_update(app->scene, PHYSICS_DT);
			
			accumulator -= PHYSICS_DT;
			app->accumulated_time += PHYSICS_DT;
		}
		app->phys_ms = (get_time_seconds() - t0) * 1000.0;

		/* Rendering */
		t0 = get_time_seconds();

		int win_w, win_h;
		bhs_ui_get_size(app->ui, &win_w, &win_h);

		/* Limpa tela - preto absoluto */
		bhs_ui_clear(app->ui, (struct bhs_ui_color){ 0.0f, 0.0f, 0.0f, 1.0f });

		/* [NEW] Dispatch Compute Pass */
		bhs_gpu_texture_t bh_tex = NULL;
		if (app->bh_pass && app->scenario == APP_SCENARIO_KERR_BLACKHOLE) {
			/* Resize check */
			bhs_blackhole_pass_resize(app->bh_pass, win_w, win_h);
			
			/* Dispatch */
			bhs_gpu_cmd_buffer_t cmd = bhs_ui_get_current_cmd(app->ui);
			bhs_blackhole_pass_dispatch(app->bh_pass, cmd, app->scene, &app->camera);
			
			bh_tex = bhs_blackhole_pass_get_output(app->bh_pass);
		}

		/* Desenha cena */
		bhs_view_assets_t assets = {
			.bg_texture = app->bg_tex,
			.sphere_texture = app->sphere_tex,
			.bh_texture = bh_tex, /* [NEW] */
			.show_grid = app->hud.show_grid,
			.tex_cache = (const struct bhs_planet_tex_entry *)app->tex_cache,
			.tex_cache_count = app->tex_cache_count
		};
		/* app_run call update */
		bhs_view_spacetime_draw(app->ui, app->scene, &app->camera,
					win_w, win_h, &assets, app->planet_pass);

/* app_shutdown unused var removal */
	/* 4.2. Black Hole Pass (Init) - removed logic if present or check context. 
	   Wait, error was in app_shutdown line 376. 
	   The logical block there was creating 'dev' but not using it?
	   Let's just remove the line. 
	*/
	/* Note: The context of 'unused variable dev' was likely inside the '4.2' block I copy-pasted in replace_file earlier?
	   No, error said 'app_shutdown'.
	   Let's target the exact lines.
	*/

		/* HUD */
		bhs_hud_draw(app->ui, &app->hud, win_w, win_h);

		/* Status bar */
		const char *status = app->sim_status == APP_SIM_PAUSED 
			? "PAUSED" : "Running";
		char status_buf[128];
		snprintf(status_buf, sizeof(status_buf),
			 "Status: %s | Time Scale: %.1fx | S=Save L=Load Space=Pause",
			 status, app->time_scale);
		bhs_ui_draw_text(app->ui, status_buf, 10, (float)win_h - 30, 
				 16.0f, BHS_UI_COLOR_GRAY);

		app->render_ms = (get_time_seconds() - t0) * 1000.0;

		/* End frame */
		bhs_ui_end_frame(app->ui);

		/* Telemetria periódica */
		app->frame_count++;
		if (app->frame_count % 30 == 0) {
			bhs_telemetry_print_scene(app->scene, app->accumulated_time,
						  app->hud.show_grid, 
						  app->phys_ms, app->render_ms);
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


/* ... Shutdown ... */

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
