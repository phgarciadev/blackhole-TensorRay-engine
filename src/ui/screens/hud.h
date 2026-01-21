#ifndef BHS_CMD_UI_HUD_H
#define BHS_CMD_UI_HUD_H

#include "gui/ui/lib.h"

#include "engine/scene/scene.h"
#include "src/simulation/data/planet.h" // Added as per instruction
#include "src/ui/screens/view_spacetime.h" /* Enum bhs_visual_mode_t */

typedef struct {
	bool show_fps;
	bool vsync_enabled;
	float time_scale_val;    /* [NEW] Slider 0..1 (Logarithmic) */
	int active_menu_index; /* -1 = none, 0=Config, 1=Add, 2=Visual */
	int add_menu_category; /* -1 = Root, 0=Planets, 1=Suns, 2=Moons, 3=BlackHoles */
	
	/* Visualization Mode */
	bhs_visual_mode_t visual_mode;
	bool top_down_view; /* [NEW] */

	/* Selection State */
	int selected_body_index;	     /* -1 = none */
	struct bhs_body selected_body_cache; /* Para exibir info sem lock */

	/* Requests to Main Loop */
	bool req_delete_body;
	int req_add_body_type; /* -1 = none */
	const struct bhs_planet_registry_entry *req_add_registry_entry; /* NULL = none */

    /* Data for Object Inspector (Calculated in app_state) */
    char attractor_name[64];
    double attractor_dist;
} bhs_hud_state_t;

void bhs_hud_init(bhs_hud_state_t *state);
void bhs_hud_draw(bhs_ui_ctx_t ctx, bhs_hud_state_t *state, int window_w,
		  int window_h);

/* Retorna true se o mouse estiver sobre algum elemento da HUD */
bool bhs_hud_is_mouse_over(const bhs_hud_state_t *state, int mx, int my,
			   int win_w, int win_h);

#endif
