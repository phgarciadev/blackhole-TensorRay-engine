/**
 * @file main.c
 * @brief Ponto de entrada do Black Hole Simulator
 *
 * "Onde tudo começa. E se der segfault, onde tudo termina."
 */

#include <stdio.h>
#include <stdlib.h>

#include "cmd/ui/screens/hud.h"
#include "cmd/ui/screens/view_spacetime.h"
#include "engine/scene/scene.h"
#include "hal/gpu/renderer.h"
#include "lib/loader/image_loader.h"
#include "cmd/debug/telemetry.h"

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	setbuf(stdout, NULL);

	printf("=== Black Hole Simulator ===\n");
	printf("Inicializando universo...\n");

	/* 1. Cria a Cena (Física) */
	bhs_scene_t scene = bhs_scene_create();
	if (!scene) {
		fprintf(stderr, "Erro fatal: Falha ao criar cena. Universo "
				"colapsou.\n");
		return 1;
	}
	bhs_scene_init_default(scene);

	/* 2. Cria Contexto UI (Janela + GPU) */
	struct bhs_ui_config config = {
		.title = "Black Hole Simulator - Spacetime View",
		.width = 1280,
		.height = 720,
		.resizable = true,
		.vsync = true,
		.debug = true /* Ativa validação pra gente ver as cacas */
	};

	bhs_ui_ctx_t ui = NULL;
	int ret = bhs_ui_create(&config, &ui);
	if (ret != BHS_UI_OK) {
		fprintf(stderr,
			"Erro fatal: Falha ao criar UI (%d). Sem placa de "
			"video?\n",
			ret);
		bhs_scene_destroy(scene);
		return 1;
	}

	/* 3. Inicializa Camera */
	bhs_camera_t cam;
	bhs_camera_init(&cam);

	/* 3.1 Inicializa HUD */
	bhs_hud_state_t hud_state;
	bhs_hud_init(&hud_state);

	/* 3.5 Carrega Textura do Espaço (Kernel-style: Fail fast) */
	printf("Carregando texturas...\n");
	bhs_image_t bg_img = bhs_image_load("assets/textures/space_bg.png");
	bhs_gpu_texture_t bg_tex = NULL;

	if (bg_img.data) {
		struct bhs_gpu_texture_config tex_conf = {
			.width = bg_img.width,
			.height = bg_img.height,
			.depth = 1,
			.mip_levels = 1,
			.array_layers = 1,
			.format =
				BHS_FORMAT_RGBA8_SRGB, /* Texture is sRGB, GPU must linearize */
			.usage = BHS_TEXTURE_SAMPLED | BHS_TEXTURE_TRANSFER_DST,
			.label = "Skybox"
		};

		bhs_gpu_device_t dev = bhs_ui_get_gpu_device(ui);
		if (bhs_gpu_texture_create(dev, &tex_conf, &bg_tex) ==
		    BHS_GPU_OK) {
			bhs_gpu_texture_upload(bg_tex, 0, 0, bg_img.data,
					       bg_img.width * bg_img.height *
						       4);
			printf("Textura carregada: %dx%d\n", bg_img.width,
			       bg_img.height);
		} else {
			fprintf(stderr, "Falha ao criar textura na GPU.\n");
		}
		bhs_image_free(bg_img); /* RAM free */
	} else {
		fprintf(stderr, "Aviso: Textura do espaco nao encontrada.\n");
	}

	/* 3.6 Gera Textura de Esfera Procedural (3D Impostor) */
	bhs_gpu_texture_t sphere_tex = NULL;
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
				.format =
					BHS_FORMAT_RGBA8_UNORM, /* Linear for masks */
				.usage = BHS_TEXTURE_SAMPLED |
					 BHS_TEXTURE_TRANSFER_DST,
				.label = "Sphere Impostor"
			};
			bhs_gpu_device_t dev = bhs_ui_get_gpu_device(ui);
			if (bhs_gpu_texture_create(dev, &conf, &sphere_tex) ==
			    BHS_GPU_OK) {
				bhs_gpu_texture_upload(sphere_tex, 0, 0,
						       sphere_img.data,
						       size * size * 4);
				printf("Esfera 3D gerada: %dx%d\n", size, size);
			}
			bhs_image_free(sphere_img);
		}
	}

	printf("Sistema online. Entrando no horizonte de eventos...\n");

	/* 4. Loop Principal */
	/* Time */
	double dt = 0.016; /* 60 FPS fixo */

	/* Loop */
	/* --- Helper: Projection for Picking (Duplicated from Renderer for simplicity) --- */
	auto void project_point(const bhs_camera_t *c, float x, float y,
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
	};

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
		if (hud_state.req_delete_body) {
			if (hud_state.selected_body_index != -1) {
				bhs_scene_remove_body(
					scene, hud_state.selected_body_index);
				hud_state.selected_body_index = -1;
			}
			hud_state.req_delete_body = false;
		}

		/* 2. Handle Object Injection */
		if (hud_state.req_add_body_type != -1) {
			/* Add slightly in front of camera */
			float spawn_dist = 20.0f;
			struct bhs_vec3 pos = {
				.x = cam.x + sinf(cam.yaw) * spawn_dist,
				.y = 0.0f, /* Flattened to accretion disk plane by default */
				.z = cam.z + cosf(cam.yaw) * spawn_dist
			};
			struct bhs_vec3 col = { (float)rand() / RAND_MAX,
						(float)rand() / RAND_MAX,
						(float)rand() / RAND_MAX };

			double mass = 0.1;
			double radius = 0.5;

			struct bhs_vec3 vel = { 0, 0, 0 };

			/*
			 * [AUTO-ORBIT] Calcula velocidade orbital automaticamente
			 * Assume órbita circular ao redor da origem (0,0,0)
			 * v = sqrt(G * M_central / r)
			 */
			if (hud_state.req_add_body_type == BHS_BODY_PLANET) {
				/* Procura massa central (primeiro BH ou Star) */
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
					/* Distância do centro */
					double r = sqrt(pos.x * pos.x + pos.z * pos.z);
					if (r > 0.1) {
						/* v_orbital = sqrt(G*M/r), G=1 */
						double v_orb = sqrt(central_mass / r);
						
						/* Direção tangencial (perpendicular ao raio) */
						double dir_x = -pos.z / r;
						double dir_z = pos.x / r;
						
						vel.x = dir_x * v_orb;
						vel.z = dir_z * v_orb;
						
						printf("[SPAWN] Planeta em r=%.2f, v_orb=%.3f "
						       "(central_mass=%.2f)\n", 
						       r, v_orb, central_mass);
					}
				} else {
					printf("[SPAWN] AVISO: Sem massa central. "
					       "Planeta vai flutuar parado.\n");
				}
			} else if (hud_state.req_add_body_type ==
				   BHS_BODY_STAR) {
				mass = 2.0;
				radius = 1.0;
				col = (struct bhs_vec3){ 1.0, 0.8, 0.2 };
				printf("[SPAWN] Estrela (mass=%.2f)\n", mass);
			} else if (hud_state.req_add_body_type ==
				   BHS_BODY_BLACKHOLE) {
				mass = 10.0; /* A bit massive to be the center */
				radius = 2.0;
				col = (struct bhs_vec3){ 0.0, 0.0, 0.0 };
				printf("[SPAWN] Buraco Negro (mass=%.2f)\n", mass);
			}

			bhs_scene_add_body(scene, hud_state.req_add_body_type,
					   pos, vel, mass, radius, col);

			hud_state.req_add_body_type = -1;
		}

		/* 3. Handle Picking (Selection) */
		if (bhs_ui_mouse_clicked(ui, 0)) { /* Left Click */
			/* Check if we clicked on UI? Simple assumption: if mouse Y < 30 (top bar) or inside panel, ignore world pick.
         * For now, just simplistic world pick.
         */
			int mx, my;
			bhs_ui_mouse_pos(ui, &mx, &my);

			/* 3.0 Check UI Block */
			if (bhs_hud_is_mouse_over(&hud_state, mx, my, win_w,
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
			hud_state.selected_body_index = best_idx;
		}
skip_picking:

		/* 4. Update Cache for HUD */
		if (hud_state.selected_body_index != -1) {
			int n;
			const struct bhs_body *b =
				bhs_scene_get_bodies(scene, &n);
			if (hud_state.selected_body_index < n) {
				hud_state.selected_body_cache =
					b[hud_state.selected_body_index];
			} else {
				hud_state.selected_body_index =
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
					     .show_grid = hud_state.show_grid };
		bhs_view_spacetime_draw(ui, scene, &cam, win_w, win_h, &assets);

		/* Interface Adicional (HUD) */
		bhs_hud_draw(ui, &hud_state, win_w, win_h);

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
						  hud_state.show_grid);
		}
	}

	printf("Desligando simulacao...\n");

	/* 4. Cleanup */
	if (bg_tex)
		bhs_gpu_texture_destroy(bg_tex);
	bhs_ui_destroy(ui);
	bhs_scene_destroy(scene);

	return 0;
}
