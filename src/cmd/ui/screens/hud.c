#include "hud.h"
#include <stdio.h>

static const char *MENU_ITEMS[] = { "Config", "Add" };
static const int MENU_COUNT = 2;

void bhs_hud_init(bhs_hud_state_t *state)
{
	if (state) {
		state->show_fps = true;
		state->vsync_enabled = true;
		state->show_grid = false; /* Começa desligado */
		state->active_menu_index = -1;
		state->selected_body_index = -1;
		state->req_delete_body = false;
		state->req_add_body_type = -1;
	}
}

void bhs_hud_draw(bhs_ui_ctx_t ctx, bhs_hud_state_t *state, int window_w,
		  int window_h)
{
	/* 1. VS Code Top Bar Background (#1e1e1e) */
	struct bhs_ui_rect bar_rect = {
		.x = 0, .y = 0, .width = (float)window_w, .height = 30.0f
	};
	bhs_ui_draw_rect(ctx, bar_rect,
			 (struct bhs_ui_color){ 0.117f, 0.117f, 0.117f, 1.0f });

	/* 2. Menu Items */
	float x_cursor = 10.0f;

	/* Draw Icon/Logo placeholder (VS Code Blue #007acc approx 0, 0.48, 0.8) */
	struct bhs_ui_rect icon_rect = {
		.x = 0, .y = 0, .width = 35.0f, .height = 30.0f
	};
	bhs_ui_draw_rect(ctx, icon_rect,
			 (struct bhs_ui_color){ 0.0f, 0.48f, 0.8f, 1.0f });

	x_cursor += 35.0f; /* Shift menu items after icon */

	for (int i = 0; i < MENU_COUNT; i++) {
		/* Dynamic width calculation to prevent overlap */
		/* Assuming approx 10px per char (safe oversize) + 20px padding */
		int len = 0;
		while (MENU_ITEMS[i][len] != '\0')
			len++;

		float width = (float)(len * 9 + 20);
		struct bhs_ui_rect item_rect = { x_cursor, 0, width, 30.0f };

		/* Hover/Active Effect */
		/* Draw a slightly lighter background if active */
		if (state->active_menu_index == i) {
			bhs_ui_draw_rect(ctx, item_rect,
					 (struct bhs_ui_color){ 0.2f, 0.2f,
								0.2f, 1.0f });
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

	/* 3. Dropdown Menu */
	if (state->active_menu_index != -1) {
		float dropdown_x =
			10.0f + 35.0f +
			(state->active_menu_index == 1 ? 80.0f : 0.0f);
		struct bhs_ui_rect panel_rect = { dropdown_x, 30, 200, 150 };

		/* VS Code Menu Bg (#252526) */
		bhs_ui_panel(
			ctx, panel_rect,
			(struct bhs_ui_color){ 0.145f, 0.145f, 0.145f, 1.0f },
			(struct bhs_ui_color){ 0.3f, 0.3f, 0.3f,
					       1.0f }); /* Border */

		float y = 35.0f;

		/* INDEX 0: CONFIG */
		if (state->active_menu_index == 0) {
			bhs_ui_draw_text(ctx, "Appearance", panel_rect.x + 10,
					 y, 14.0f, BHS_UI_COLOR_GRAY);
			y += 25.0f;

			struct bhs_ui_rect item_rect = { panel_rect.x + 10, y,
							 180, 24 };
			bhs_ui_checkbox(ctx, "Show FPS", item_rect,
					&state->show_fps);
			
			y += 28.0f;
			item_rect.y = y;
			bhs_ui_checkbox(ctx, "Show Grid", item_rect,
					&state->show_grid);

			y += 28.0f;
			item_rect.y = y;
			bool vsync_prev = state->vsync_enabled;
			bhs_ui_checkbox(ctx, "Enable VSync", item_rect,
					&state->vsync_enabled);
			if (state->vsync_enabled != vsync_prev) {
				printf("[HUD] VSync changed (Restart "
				       "required)\n");
			}
		}
		/* INDEX 1: ADD */
		else if (state->active_menu_index == 1) {
			bhs_ui_draw_text(ctx, "Inject Body", panel_rect.x + 10,
					 y, 14.0f, BHS_UI_COLOR_GRAY);
			y += 25.0f;

			struct bhs_ui_rect btn_rect = { panel_rect.x + 10, y,
							180, 24 };
			if (bhs_ui_button(ctx, "Planet (Random)", btn_rect)) {
				state->req_add_body_type = BHS_BODY_PLANET;
				state->active_menu_index = -1; /* Close menu */
			}

			y += 28.0f;
			btn_rect.y = y;
			if (bhs_ui_button(ctx, "Star", btn_rect)) {
				state->req_add_body_type = BHS_BODY_STAR;
				state->active_menu_index = -1;
			}

			y += 28.0f;
			btn_rect.y = y;
			if (bhs_ui_button(ctx, "Black Hole", btn_rect)) {
				state->req_add_body_type = BHS_BODY_BLACKHOLE;
				state->active_menu_index = -1;
			}
		}
	}

	/* 4. Info Panel (Expanded) */
	if (state->selected_body_index != -1) {
		/* Aumentar altura do painel para caber dados */
		struct bhs_ui_rect info_rect = { (float)window_w - 260, 40, 240,
						 350 };
		bhs_ui_panel(ctx, info_rect,
			     (struct bhs_ui_color){ 0.1f, 0.1f, 0.1f, 0.9f },
			     (struct bhs_ui_color){ 0.0f, 0.48f, 0.8f,
						    0.5f }); /* Blue Border */

		float y = 50.0f;
		float x = info_rect.x + 10;
		const struct bhs_body *b = &state->selected_body_cache;

		/* Header */
		bhs_ui_draw_text(ctx, "Object Inspector", x, y, 16.0f,
				 BHS_UI_COLOR_WHITE);
		y += 25.0f;

		/* --- UNIVERSAL DATA --- */
		char buf[128];
		
		const char* type_str = "Unknown";
		if (b->type == BHS_BODY_PLANET) type_str = "Planet (Rocky/Gas)";
		else if (b->type == BHS_BODY_STAR) type_str = "Star (Plasma)";
		else if (b->type == BHS_BODY_BLACKHOLE) type_str = "Black Hole (Singularity)";
		
		bhs_ui_draw_text(ctx, type_str, x, y, 14.0f, BHS_UI_COLOR_WHITE);
		y += 20.0f;

		snprintf(buf, sizeof(buf), "Mass: %.3e kg", b->state.mass);
		bhs_ui_draw_text(ctx, buf, x, y, 13.0f, BHS_UI_COLOR_GRAY); 
		y += 16.0f;

		snprintf(buf, sizeof(buf), "Radius: %.3e m", b->state.radius);
		bhs_ui_draw_text(ctx, buf, x, y, 13.0f, BHS_UI_COLOR_GRAY);
		y += 16.0f;

		snprintf(buf, sizeof(buf), "Pos: (%.1f, %.1f)", b->state.pos.x, b->state.pos.z);
		bhs_ui_draw_text(ctx, buf, x, y, 13.0f, BHS_UI_COLOR_GRAY);
		y += 16.0f;

		snprintf(buf, sizeof(buf), "Vel: (%.3f, %.3f)", b->state.vel.x, b->state.vel.z);
		bhs_ui_draw_text(ctx, buf, x, y, 13.0f, BHS_UI_COLOR_GRAY);
		y += 20.0f;

		/* --- TYPE SPECIFIC --- */
		bhs_ui_draw_text(ctx, "--- Properties ---", x, y, 14.0f, BHS_UI_COLOR_WHITE);
		y += 20.0f;

		if (b->type == BHS_BODY_PLANET) {
			snprintf(buf, sizeof(buf), "Density: %.0f kg/m3", b->prop.planet.density);
			bhs_ui_draw_text(ctx, buf, x, y, 13.0f, BHS_UI_COLOR_GRAY); y += 16.0f;
			
			snprintf(buf, sizeof(buf), "Temp: %.1f K", b->prop.planet.temperature);
			bhs_ui_draw_text(ctx, buf, x, y, 13.0f, BHS_UI_COLOR_GRAY); y += 16.0f;

			snprintf(buf, sizeof(buf), "Pressure: %.2f atm", b->prop.planet.surface_pressure);
			bhs_ui_draw_text(ctx, buf, x, y, 13.0f, BHS_UI_COLOR_GRAY); y += 16.0f;

			snprintf(buf, sizeof(buf), "Atmos: %s", b->prop.planet.has_atmosphere ? "Yes" : "No");
			bhs_ui_draw_text(ctx, buf, x, y, 13.0f, BHS_UI_COLOR_GRAY); y += 16.0f;

			snprintf(buf, sizeof(buf), "Comp: %s", b->prop.planet.composition);
			bhs_ui_draw_text(ctx, buf, x, y, 13.0f, BHS_UI_COLOR_GRAY); y += 16.0f;
		}
		else if (b->type == BHS_BODY_STAR) {
			snprintf(buf, sizeof(buf), "Lum: %.2e W", b->prop.star.luminosity);
			bhs_ui_draw_text(ctx, buf, x, y, 13.0f, BHS_UI_COLOR_GRAY); y += 16.0f;

			snprintf(buf, sizeof(buf), "Teff: %.0f K", b->prop.star.temp_effective);
			bhs_ui_draw_text(ctx, buf, x, y, 13.0f, BHS_UI_COLOR_GRAY); y += 16.0f;

			snprintf(buf, sizeof(buf), "Type: %s", b->prop.star.spectral_type);
			bhs_ui_draw_text(ctx, buf, x, y, 13.0f, BHS_UI_COLOR_GRAY); y += 16.0f;
			
			snprintf(buf, sizeof(buf), "Age: %.1e yr", b->prop.star.age);
			bhs_ui_draw_text(ctx, buf, x, y, 13.0f, BHS_UI_COLOR_GRAY); y += 16.0f;
		}
		else if (b->type == BHS_BODY_BLACKHOLE) {
			snprintf(buf, sizeof(buf), "Spin (a): %.2f", b->prop.bh.spin_factor);
			bhs_ui_draw_text(ctx, buf, x, y, 13.0f, BHS_UI_COLOR_GRAY); y += 16.0f;

			snprintf(buf, sizeof(buf), "R_Schwarz: %.2f", b->prop.bh.event_horizon_r);
			bhs_ui_draw_text(ctx, buf, x, y, 13.0f, BHS_UI_COLOR_GRAY); y += 16.0f;
		}
		
		y += 10.0f; /* Spacing before buttons */

		/* Delete Button */
		struct bhs_ui_rect del_rect = { x, y, 100, 24 };
		/* Red button backing */
		bhs_ui_draw_rect(
			ctx, del_rect,
			(struct bhs_ui_color){ 0.6f, 0.2f, 0.2f, 1.0f });
		if (bhs_ui_button(ctx, "Apagar", del_rect)) {
			state->req_delete_body = true;
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

/* Helper: AABB Check */
static bool is_inside(int mx, int my, float x, float y, float w, float h)
{
	return (float)mx >= x && (float)mx <= x + w && (float)my >= y &&
	       (float)my <= y + h;
}

bool bhs_hud_is_mouse_over(const bhs_hud_state_t *state, int mx, int my,
			   int win_w, int win_h)
{
	/* 1. Top Bar (Always there) */
	/* Rect: 0, 0, width, 30 */
	if (is_inside(mx, my, 0, 0, (float)win_w, 30.0f))
		return true;

	/* 2. Dropdown Menu */
	if (state->active_menu_index != -1) {
		float dropdown_x =
			10.0f + 35.0f +
			(state->active_menu_index == 1 ? 80.0f : 0.0f);
		/* Rect: dropdown_x, 30, 200, 150 */
		if (is_inside(mx, my, dropdown_x, 30.0f, 200.0f, 150.0f))
			return true;
	}

	/* 3. Info Panel (Expanded) */
	if (state->selected_body_index != -1) {
		/* Rect: win_w - 260, 40, 240, 350 */
		if (is_inside(mx, my, (float)win_w - 260.0f, 40.0f, 240.0f,
			      350.0f))
			return true;
	}

	return false;
}
