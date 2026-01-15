/**
 * @file window.c
 * @brief Gerenciamento de Janelas (Wrapper)
 *
 * Aqui a gente encapsula a criação da janela e do swapchain.
 * Porque ninguém merece ver aquele monte de configs do Vulkan no main context.
 *
 * "Janelas são os olhos da alma... do seu computador."
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "framework/ui/internal.h"

/* Callback interno de eventos */
static void ui_event_callback(bhs_window_t window,
                              const struct bhs_event *event, void *userdata) {
  (void)window;
  struct bhs_ui_ctx_impl *ctx = userdata;
  if (!ctx)
    return;

  switch (event->type) {
  case BHS_EVENT_WINDOW_CLOSE:
    ctx->should_close = true;
    break;

  case BHS_EVENT_WINDOW_RESIZE:
    ctx->width = event->resize.width;
    ctx->height = event->resize.height;

    /* Avisa renderer para recriar swapchain */
    if (ctx->swapchain) {
      bhs_gpu_swapchain_resize(ctx->swapchain, (uint32_t)ctx->width,
                               (uint32_t)ctx->height);
    }
    break;

  case BHS_EVENT_KEY_DOWN:
  case BHS_EVENT_KEY_REPEAT:
    if (event->key.scancode < BHS_UI_MAX_KEYS) {
      ctx->input.keys[event->key.scancode] = true;
    }
    break;

  case BHS_EVENT_KEY_UP:
    if (event->key.scancode < BHS_UI_MAX_KEYS) {
      ctx->input.keys[event->key.scancode] = false;
    }
    break;

  case BHS_EVENT_MOUSE_MOVE:
    ctx->input.mouse_x = event->mouse_move.x;
    ctx->input.mouse_y = event->mouse_move.y;
    break;

  case BHS_EVENT_MOUSE_DOWN:
    if (event->mouse_button.button < BHS_UI_MAX_BUTTONS) {
      ctx->input.buttons[event->mouse_button.button] = true;
    }
    break;

  case BHS_EVENT_MOUSE_UP:
    if (event->mouse_button.button < BHS_UI_MAX_BUTTONS) {
      ctx->input.buttons[event->mouse_button.button] = false;
    }
    break;

  default:
    break;
  }
}

int bhs_ui_window_init_internal(bhs_ui_ctx_t ctx,
                                const struct bhs_ui_config *config) {
  if (!ctx || !config)
    return BHS_UI_ERR_INVALID;

  int ret;

  /* === Inicializa Platform === */
  ret = bhs_platform_init(&ctx->platform);
  if (ret != BHS_PLATFORM_OK) {
    fprintf(stderr, "[ui/window] erro: falha ao inicializar platform (%d)\n",
            ret);
    return BHS_UI_ERR_INIT;
  }

  /* === Cria Janela === */
  struct bhs_window_config win_config = {
      .title = config->title ? config->title : "Black Hole Simulator",
      .width = config->width > 0 ? config->width : 800,
      .height = config->height > 0 ? config->height : 600,
      .x = BHS_WINDOW_POS_CENTERED,
      .y = BHS_WINDOW_POS_CENTERED,
      .flags = config->resizable ? BHS_WINDOW_RESIZABLE : 0,
  };

  ret = bhs_window_create(ctx->platform, &win_config, &ctx->window);
  if (ret != BHS_PLATFORM_OK) {
    fprintf(stderr, "[ui/window] erro: falha ao criar janela (%d)\n", ret);
    bhs_platform_shutdown(ctx->platform);
    ctx->platform = NULL;
    return BHS_UI_ERR_WINDOW;
  }

  ctx->width = win_config.width;
  ctx->height = win_config.height;

  /* Registra callback */
  bhs_window_set_event_callback(ctx->window, ui_event_callback, ctx);

  return BHS_UI_OK;
}

void bhs_ui_window_shutdown_internal(bhs_ui_ctx_t ctx) {
  if (!ctx)
    return;

  if (ctx->window) {
    bhs_window_destroy(ctx->window);
    ctx->window = NULL;
  }
  if (ctx->platform) {
    bhs_platform_shutdown(ctx->platform);
    ctx->platform = NULL;
  }
}

void bhs_ui_window_poll_events(bhs_ui_ctx_t ctx) {
  if (!ctx || !ctx->platform)
    return;
  bhs_platform_poll_events(ctx->platform);
}
