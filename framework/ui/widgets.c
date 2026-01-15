/**
 * @file widgets.c
 * @brief Implementação de Widgets UI
 */

#include "framework/assert.h"
#include "framework/ui/internal.h"
#include "framework/ui/layout.h"
#include <string.h>

/* ============================================================================
 * HELPERS
 * ============================================================================
 */

static bool rect_contains(struct bhs_ui_rect rect, float px, float py) {
  return px >= rect.x && px < rect.x + rect.width && py >= rect.y &&
         py < rect.y + rect.height;
}

/* ============================================================================
 * SÍMBOLOS PROCEDURAIS
 * ============================================================================
 */

static void draw_icon(bhs_ui_ctx_t ctx, enum bhs_ui_icon icon, float x, float y,
                      float size, struct bhs_ui_color color) {
  float pad = size * 0.2f;
  float s = size - pad * 2.0f;

  switch (icon) {
  case BHS_ICON_GEAR: {
    /* Círculo central + "dentes" (quadradinhos) */
    bhs_ui_draw_rect(ctx,
                     (struct bhs_ui_rect){x + size * 0.4f, y + pad, size * 0.2f,
                                          size * 0.8f},
                     color);
    bhs_ui_draw_rect(ctx,
                     (struct bhs_ui_rect){x + pad, y + size * 0.4f, size * 0.8f,
                                          size * 0.2f},
                     color);
    /* Octagon-ish layout */
    float d = size * 0.25f;
    bhs_ui_draw_rect(
        ctx, (struct bhs_ui_rect){x + d, y + d, size * 0.5f, size * 0.5f},
        color);
    /* Furo central */
    bhs_ui_draw_rect(ctx,
                     (struct bhs_ui_rect){x + size * 0.45f, y + size * 0.45f,
                                          size * 0.1f, size * 0.1f},
                     BHS_UI_COLOR_BLACK);
    break;
  }
  case BHS_ICON_PHYSICS: {
    /* Átomo estilizado (3 elipses/retângulos rotacionados) */
    bhs_ui_draw_rect_outline(
        ctx, (struct bhs_ui_rect){x + pad, y + size * 0.4f, s, size * 0.2f},
        color, 1.0f);
    bhs_ui_draw_rect_outline(
        ctx, (struct bhs_ui_rect){x + size * 0.4f, y + pad, size * 0.2f, s},
        color, 1.0f);
    bhs_ui_draw_rect(ctx,
                     (struct bhs_ui_rect){x + size * 0.45f, y + size * 0.45f,
                                          size * 0.1f, size * 0.1f},
                     color);
    break;
  }
  case BHS_ICON_CAMERA: {
    /* Corpo da câmera + lente */
    bhs_ui_draw_rect(
        ctx, (struct bhs_ui_rect){x + pad, y + size * 0.4f, s, size * 0.4f},
        color);
    bhs_ui_draw_rect(ctx,
                     (struct bhs_ui_rect){x + size * 0.35f, y + size * 0.3f,
                                          size * 0.3f, size * 0.1f},
                     color);
    bhs_ui_draw_rect_outline(ctx,
                             (struct bhs_ui_rect){x + size * 0.4f,
                                                  y + size * 0.5f, size * 0.2f,
                                                  size * 0.2f},
                             BHS_UI_COLOR_BLACK, 1.0f);
    break;
  }
  case BHS_ICON_CLOSE: {
    /* X */
    float t = 2.0f;
    /* Simplificado por rects cruzados */
    bhs_ui_draw_rect(
        ctx, (struct bhs_ui_rect){x + pad, y + size * 0.5f - t / 2, s, t},
        color);
    /* Como não temos rotação fácil no draw_rect ainda, vamos improvisar com um
     * mini-box central */
    bhs_ui_draw_rect(
        ctx, (struct bhs_ui_rect){x + size * 0.5f - t / 2, y + pad, t, s},
        color);
    break;
  }
  default:
    break;
  }
}

/* ============================================================================
 * WIDGETS
 * ============================================================================
 */

bool bhs_ui_icon_button(bhs_ui_ctx_t ctx, enum bhs_ui_icon icon, float x,
                        float y, float size) {
  BHS_ASSERT(ctx != NULL);

  struct bhs_ui_rect rect = {x, y, size, size};
  int32_t mx, my;
  bhs_ui_mouse_pos(ctx, &mx, &my);

  bool hovered = rect_contains(rect, (float)mx, (float)my);
  struct bhs_ui_color bg = {0.15f, 0.15f, 0.2f, 0.8f};
  struct bhs_ui_color ic_color = BHS_UI_COLOR_WHITE;

  if (hovered && bhs_ui_mouse_down(ctx, 0)) {
    bg = (struct bhs_ui_color){0.3f, 0.3f, 0.5f, 1.0f};
  } else if (hovered) {
    bg = (struct bhs_ui_color){0.25f, 0.25f, 0.35f, 0.9f};
    ic_color = (struct bhs_ui_color){0.8f, 0.9f, 1.0f, 1.0f};
  }

  /* Fundo circular (estilizado como rect com borda por enquanto) */
  bhs_ui_draw_rect(ctx, rect, bg);
  bhs_ui_draw_rect_outline(ctx, rect, ic_color, 1.0f);

  draw_icon(ctx, icon, x, y, size, ic_color);

  return hovered && bhs_ui_mouse_clicked(ctx, 0);
}

void bhs_ui_panel_begin(bhs_ui_ctx_t ctx, const char *title, float width,
                        float height) {
  BHS_ASSERT(ctx != NULL);

  int32_t win_w, win_h;
  bhs_ui_get_size(ctx, &win_w, &win_h);

  /* 1. Overlay escuro */
  bhs_ui_draw_rect(ctx, (struct bhs_ui_rect){0, 0, (float)win_w, (float)win_h},
                   (struct bhs_ui_color){0.0f, 0.0f, 0.0f, 0.6f});

  /* 2. Janela Centralizada */
  float x = (win_w - width) / 2.0f;
  float y = (win_h - height) / 2.0f;
  struct bhs_ui_rect rect = {x, y, width, height};

  bhs_ui_draw_rect(ctx, rect, (struct bhs_ui_color){0.12f, 0.12f, 0.15f, 1.0f});
  bhs_ui_draw_rect_outline(ctx, rect,
                           (struct bhs_ui_color){0.4f, 0.4f, 0.5f, 1.0f}, 2.0f);

  /* Title bar sutil */
  bhs_ui_draw_rect(ctx, (struct bhs_ui_rect){x, y, width, 30.0f},
                   (struct bhs_ui_color){0.2f, 0.2f, 0.25f, 1.0f});
  if (title) {
    bhs_ui_draw_text(ctx, title, x + 10, y + 8, 14.0f, BHS_UI_COLOR_WHITE);
  }

  /* Inicia Layout dentro da janela (com padding) */
  struct bhs_layout_style style = {0};
  style.padding[0] = 40.0f; /* Top (after title bar) */
  style.padding[1] = 20.0f; /* Right */
  style.padding[2] = 20.0f; /* Bottom */
  style.padding[3] = 20.0f; /* Left */
  style.gap = 10.0f;

  /* Ajusta retângulo de layout */
  ctx->layout.stack[0].rect = rect;
  bhs_layout_begin(ctx, BHS_LAYOUT_COLUMN, &style);
}

void bhs_ui_panel_end(bhs_ui_ctx_t ctx) {
  BHS_ASSERT(ctx != NULL);
  bhs_layout_end(ctx);
}

bool bhs_ui_checkbox(bhs_ui_ctx_t ctx, const char *label,
                     struct bhs_ui_rect rect, bool *checked) {
  BHS_ASSERT(ctx != NULL);
  BHS_ASSERT(checked != NULL);

  /* Checkbox Square */
  struct bhs_ui_rect box = {rect.x, rect.y, rect.height, rect.height};
  bhs_ui_draw_rect(ctx, box, (struct bhs_ui_color){0.1f, 0.1f, 0.1f, 1.0f});
  bhs_ui_draw_rect_outline(ctx, box,
                           (struct bhs_ui_color){0.5f, 0.5f, 0.6f, 1.0f}, 1.0f);

  /* Check Mark (X procedimental ou rect) */
  if (*checked)
    bhs_ui_draw_rect(ctx,
                     (struct bhs_ui_rect){box.x + 4, box.y + 4, box.width - 8,
                                          box.height - 8},
                     (struct bhs_ui_color){0.4f, 0.7f, 1.0f, 1.0f});

  /* Label */
  if (label)
    bhs_ui_draw_text(ctx, label, rect.x + rect.height + 8, rect.y + 4, 14.0f,
                     BHS_UI_COLOR_WHITE);

  /* Logic */
  int32_t mx, my;
  bhs_ui_mouse_pos(ctx, &mx, &my);
  bool hovered = rect_contains(rect, (float)mx, (float)my);

  if (hovered && bhs_ui_mouse_clicked(ctx, 0)) {
    *checked = !(*checked);
    return true;
  }
  return false;
}

void bhs_ui_label(bhs_ui_ctx_t ctx, const char *text, float x, float y) {
  BHS_ASSERT(ctx != NULL);
  BHS_ASSERT(text != NULL);
  bhs_ui_draw_text(ctx, text, x, y, 14.0f, BHS_UI_COLOR_WHITE);
}

bool bhs_ui_button(bhs_ui_ctx_t ctx, const char *label,
                   struct bhs_ui_rect rect) {
  BHS_ASSERT(ctx != NULL);

  int32_t mx, my;
  bhs_ui_mouse_pos(ctx, &mx, &my);

  bool hovered = rect_contains(rect, (float)mx, (float)my);
  struct bhs_ui_color bg;

  /* Estado Visual */
  if (hovered && bhs_ui_mouse_down(ctx, 0))
    bg = (struct bhs_ui_color){0.2f, 0.2f, 0.3f, 1.0f}; /* Active */
  else if (hovered)
    bg = (struct bhs_ui_color){0.3f, 0.3f, 0.4f, 1.0f}; /* Hover */
  else
    bg = (struct bhs_ui_color){0.25f, 0.25f, 0.35f, 1.0f}; /* Normal */

  bhs_ui_draw_rect(ctx, rect, bg);
  bhs_ui_draw_rect_outline(ctx, rect, BHS_UI_COLOR_WHITE, 1.0f);

  if (label)
    bhs_ui_draw_text(ctx, label, rect.x + 8, rect.y + 8, 16.0f,
                     BHS_UI_COLOR_WHITE);

  return hovered && bhs_ui_mouse_clicked(ctx, 0);
}

void bhs_ui_panel(bhs_ui_ctx_t ctx, struct bhs_ui_rect rect,
                  struct bhs_ui_color bg, struct bhs_ui_color border) {
  BHS_ASSERT(ctx != NULL);
  bhs_ui_draw_rect(ctx, rect, bg);
  bhs_ui_draw_rect_outline(ctx, rect, border, 1.0f);
}

bool bhs_ui_slider(bhs_ui_ctx_t ctx, struct bhs_ui_rect rect, float *value) {
  BHS_ASSERT(ctx != NULL);
  BHS_ASSERT(value != NULL);

  /* Clamping Input */
  if (*value < 0.0f)
    *value = 0.0f;
  if (*value > 1.0f)
    *value = 1.0f;

  /* Background */
  bhs_ui_draw_rect(ctx, rect, (struct bhs_ui_color){0.15f, 0.15f, 0.15f, 1.0f});

  /* Filled Part */
  struct bhs_ui_rect filled = {rect.x, rect.y, rect.width * (*value),
                               rect.height};
  bhs_ui_draw_rect(ctx, filled, (struct bhs_ui_color){0.3f, 0.5f, 0.9f, 1.0f});

  /* Interaction */
  int32_t mx, my;
  bhs_ui_mouse_pos(ctx, &mx, &my);
  bool hovered = rect_contains(rect, (float)mx, (float)my);

  if (hovered && bhs_ui_mouse_down(ctx, 0)) {
    float new_value = ((float)mx - rect.x) / rect.width;
    if (new_value < 0.0f)
      new_value = 0.0f;
    else if (new_value > 1.0f)
      new_value = 1.0f;

    if (new_value != *value) {
      *value = new_value;
      return true;
    }
  }
  return false;
}
