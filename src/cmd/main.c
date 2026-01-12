/**
 * @file main.c
 * @brief Ponto de entrada do Black Hole Simulator
 *
 * "Onde tudo começa. E se der segfault, onde tudo termina."
 */

#include <stdio.h>
#include <stdlib.h>

#include "cmd/ui/screens/view_spacetime.h"
#include "engine/scene/scene.h"

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

    /* Limpa tela (Preto profundo) */
    bhs_ui_clear(ui, (struct bhs_ui_color){0.05f, 0.05f, 0.08f, 1.0f});

    /* Desenha Malha Espacial */
    int w, h;
    bhs_ui_get_size(ui, &w, &h);
    bhs_view_spacetime_draw(ui, scene, &cam, w, h);

    /* Interface Adicional (HUD) */
    bhs_ui_draw_text(ui, "FPS: 60 (Trust me)", 10, 10, 16.0f,
                     BHS_UI_COLOR_WHITE);
    bhs_ui_draw_text(ui, "Controls: WASD (Pan), Q/E (Zoom)", 10, 30, 16.0f,
                     BHS_UI_COLOR_GRAY);

    /* Finaliza Frame */
    bhs_ui_end_frame(ui);
  }

  printf("Desligando simulacao...\n");

  /* 4. Cleanup */
  bhs_ui_destroy(ui);
  bhs_scene_destroy(scene);

  return 0;
}
