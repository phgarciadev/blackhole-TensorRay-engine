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

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  printf("=== Black Hole Simulator ===\n");
  printf("Inicializando universo...\n");

  /* 1. Cria a Cena (Física) */
  bhs_scene_t scene = bhs_scene_create();
  if (!scene) {
    fprintf(stderr, "Erro fatal: Falha ao criar cena. Universo colapsou.\n");
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
    fprintf(stderr, "Erro fatal: Falha ao criar UI (%d). Sem placa de video?\n",
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
        .label = "Skybox"};

    bhs_gpu_device_t dev = bhs_ui_get_gpu_device(ui);
    if (bhs_gpu_texture_create(dev, &tex_conf, &bg_tex) == BHS_GPU_OK) {
      bhs_gpu_texture_upload(bg_tex, 0, 0, bg_img.data,
                             bg_img.width * bg_img.height * 4);
      printf("Textura carregada: %dx%d\n", bg_img.width, bg_img.height);
    } else {
      fprintf(stderr, "Falha ao criar textura na GPU.\n");
    }
    bhs_image_free(bg_img); /* RAM free */
  } else {
    fprintf(stderr, "Aviso: Textura do espaco nao encontrada.\n");
  }

  printf("Sistema online. Entrando no horizonte de eventos...\n");

  /* 4. Loop Principal */
  /* Time */
  double dt = 0.016; /* 60 FPS fixo */

  /* Loop */
  while (!bhs_ui_should_close(ui)) {
    /* UI Framework handles polling inside begin_frame or internal loop
     * mechanisms */

    /* Inicia Frame */
    if (bhs_ui_begin_frame(ui) != BHS_UI_OK) {
      continue; /* Frame perdido, vida que segue */
    }

    /* Inicia Gravação de Comandos e Render Pass */
    bhs_ui_cmd_begin(ui);
    bhs_ui_begin_drawing(ui);

    /* Atualiza Física (dt fixo 60fps por enquanto) */
    bhs_scene_update(scene, dt);

    /* Atualiza Câmera (Input) */
    bhs_camera_update_view(&cam, ui, dt);

    /* Limpa tela (Preto absoluto para contraste máximo) */
    bhs_ui_clear(ui, (struct bhs_ui_color){0.0f, 0.0f, 0.0f, 1.0f});

    /* Desenha Malha Espacial (Passamos a textura aqui) */
    int w, h;
    bhs_ui_get_size(ui, &w, &h);

    /* DEBUG: Print size every frame to stderr */
    /* fprintf(stderr, "Size: %dx%d\n", w, h); */

    bhs_view_spacetime_draw(ui, scene, &cam, w, h, bg_tex);

    /* Interface Adicional (HUD) */
    bhs_hud_draw(ui, &hud_state, w, h);

    /* Text info inferior (permanente) */
    bhs_ui_draw_text(ui, "Status: Empty Universe (Waiting for Mass Injection)",
                     10, (float)h - 30, 16.0f, BHS_UI_COLOR_GRAY);

    /* Finaliza Frame */
    bhs_ui_end_frame(ui);
  }

  printf("Desligando simulacao...\n");

  /* 4. Cleanup */
  if (bg_tex)
    bhs_gpu_texture_destroy(bg_tex);
  bhs_ui_destroy(ui);
  bhs_scene_destroy(scene);

  return 0;
}
