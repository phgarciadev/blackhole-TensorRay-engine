#include "hud.h"
#include <stdio.h>

static const char *MENU_ITEMS[] = {"File", "Edit", "Selection", "View",
                                   "Go",   "Run",  "Terminal",  "Help"};
static const int MENU_COUNT = 8;

void bhs_hud_init(bhs_hud_state_t *state) {
  if (state) {
    state->show_fps = true;
    state->vsync_enabled = true;
    state->active_menu_index = -1;
  }
}

void bhs_hud_draw(bhs_ui_ctx_t ctx, bhs_hud_state_t *state, int window_w,
                  int window_h) {
  /* 1. VS Code Top Bar Background (#1e1e1e) */
  struct bhs_ui_rect bar_rect = {
      .x = 0, .y = 0, .width = (float)window_w, .height = 30.0f};
  bhs_ui_draw_rect(ctx, bar_rect,
                   (struct bhs_ui_color){0.117f, 0.117f, 0.117f, 1.0f});

  /* 2. Menu Items */
  float x_cursor = 10.0f;

  /* Draw Icon/Logo placeholder (VS Code Blue #007acc approx 0, 0.48, 0.8) */
  struct bhs_ui_rect icon_rect = {
      .x = 0, .y = 0, .width = 35.0f, .height = 30.0f};
  bhs_ui_draw_rect(ctx, icon_rect,
                   (struct bhs_ui_color){0.0f, 0.48f, 0.8f, 1.0f});

  x_cursor += 35.0f; /* Shift menu items after icon */

  for (int i = 0; i < MENU_COUNT; i++) {
    /* Dynamic width calculation to prevent overlap */
    /* Assuming approx 10px per char (safe oversize) + 20px padding */
    int len = 0;
    while (MENU_ITEMS[i][len] != '\0')
      len++;

    float width = (float)(len * 9 + 20);
    struct bhs_ui_rect item_rect = {x_cursor, 0, width, 30.0f};

    /* Hover/Active Effect */
    /* Draw a slightly lighter background if active */
    if (state->active_menu_index == i) {
      bhs_ui_draw_rect(ctx, item_rect,
                       (struct bhs_ui_color){0.2f, 0.2f, 0.2f, 1.0f});
    }

    if (bhs_ui_button(ctx, MENU_ITEMS[i], item_rect)) {
      if (state->active_menu_index == i) {
        state->active_menu_index = -1; /* Toggle off */
      } else {
        state->active_menu_index = i;
      }
    }

    x_cursor += width;
  }

  /* 3. Dropdown Menu (Only for 'View' currently implemented) */
  /* 'View' is index 3 */
  if (state->active_menu_index == 3) {
    /* Dropdown position: under 'View' approx x=130? Calculation is rough */
    /* File(50) + Edit(50) + Selection(70) ~ 170. Let's hardcode for View for
     * now */
    struct bhs_ui_rect panel_rect = {120, 30, 250, 150};

    /* VS Code Menu Bg (#252526 approx 0.145) */
    bhs_ui_panel(ctx, panel_rect,
                 (struct bhs_ui_color){0.145f, 0.145f, 0.145f, 1.0f},
                 (struct bhs_ui_color){0.3f, 0.3f, 0.3f, 1.0f}); /* Border */

    float y_cursor = 35.0f;
    bhs_ui_draw_text(ctx, "Appearance", panel_rect.x + 10, y_cursor, 14.0f,
                     BHS_UI_COLOR_GRAY);
    y_cursor += 20.0f;

    /* Config Items */
    struct bhs_ui_rect item_rect = {panel_rect.x + 10, y_cursor, 230, 24};

    bhs_ui_checkbox(ctx, "Toggle FPS Counter", item_rect, &state->show_fps);

    item_rect.y += 28;
    bool vsync_prev = state->vsync_enabled;
    bhs_ui_checkbox(ctx, "Enable VSync", item_rect, &state->vsync_enabled);

    if (state->vsync_enabled != vsync_prev) {
      printf("[HUD] VSync changed (Requires restart)\n");
    }
  }

  /* 4. FPS Counter (Overlay - independent of menu) */
  if (state->show_fps) {
    char fps_text[32];
    snprintf(fps_text, 32, "FPS: 60");
    /* Draw text directly on bar or below? VS Code doesn't show FPS on bar
     * usually. */
    /* Let's keep it in the user's HUD area overlay */
    bhs_ui_draw_text(ctx, fps_text, (float)window_w - 80, 8, 16.0f,
                     BHS_UI_COLOR_WHITE);
  }
}
