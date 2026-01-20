#ifndef BHS_CMD_UI_HUD_H
#define BHS_CMD_UI_HUD_H

#include "gui-framework/ui/lib.h"

#include "engine/scene/scene.h"
#include "src/simulation/data/planet.h" // Added as per instruction

typedef struct {
	bool show_fps;
	bool vsync_enabled;
	float time_scale_val;    /* [NEW] Slider 0..1 (Logarithmic) */
	int active_menu_index; /* -1 = none, 0=Config, 1=Add */

	/* Selection State */
	int selected_body_index;	     /* -1 = none */
	struct bhs_body selected_body_cache; /* Para exibir info sem lock */

	/* Requests to Main Loop */
	bool req_delete_body;
	int req_add_body_type; /* -1 = none */
	const struct bhs_planet_registry_entry *req_add_registry_entry; /* NULL = none */
} bhs_hud_state_t;

void bhs_hud_init(bhs_hud_state_t *state);
void bhs_hud_draw(bhs_ui_ctx_t ctx, bhs_hud_state_t *state, int window_w,
		  int window_h);

/* Retorna true se o mouse estiver sobre algum elemento da HUD */
bool bhs_hud_is_mouse_over(const bhs_hud_state_t *state, int mx, int my,
			   int win_w, int win_h);

#endif
