#ifndef BHS_CMD_UI_HUD_H
#define BHS_CMD_UI_HUD_H

#include "lib/ui_framework/lib.h"

typedef struct {
  bool show_fps;
  bool vsync_enabled;
  int active_menu_index; /* -1 = none, 0=File, 1=Edit, etc. */
} bhs_hud_state_t;

void bhs_hud_init(bhs_hud_state_t *state);
void bhs_hud_draw(bhs_ui_ctx_t ctx, bhs_hud_state_t *state, int window_w,
                  int window_h);

#endif
