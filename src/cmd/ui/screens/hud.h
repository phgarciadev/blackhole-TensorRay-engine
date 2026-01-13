#ifndef BHS_CMD_UI_HUD_H
#define BHS_CMD_UI_HUD_H

#include "lib/ui_framework/lib.h"

#include "engine/body/body.h"

typedef struct {
	bool show_fps;
	bool vsync_enabled;
	bool show_grid;	       /* Malha espacial visivel? */
	int active_menu_index; /* -1 = none, 0=Config, 1=Add */

	/* Selection State */
	int selected_body_index;	     /* -1 = none */
	struct bhs_body selected_body_cache; /* Para exibir info sem lock */

	/* Requests to Main Loop */
	bool req_delete_body;
	int req_add_body_type; /* -1 = none */
} bhs_hud_state_t;

void bhs_hud_init(bhs_hud_state_t *state);
void bhs_hud_draw(bhs_ui_ctx_t ctx, bhs_hud_state_t *state, int window_w,
		  int window_h);

/* Retorna true se o mouse estiver sobre algum elemento da HUD */
bool bhs_hud_is_mouse_over(const bhs_hud_state_t *state, int mx, int my,
			   int win_w, int win_h);

#endif
