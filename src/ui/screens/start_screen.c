/**
 * @file start_screen.c
 * @brief Implementação da Tela de Início "Antigravity Style" (Riemann Engine)
 */

#include "start_screen.h"
#include "app_state.h"
#include "simulation/scenario_mgr.h"
#include "gui/ui/lib.h"
#include "gui/ui/layout.h"
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * ESTILO E CORES (ANTIGRAVITY STYLE - LINEAR SPACE)
 * ============================================================================
 */

/* Background: Very Deep Dark Blue/Grey (Almost Black) */
/* sRGB ref: #101116 -> Linear: ~0.003, 0.004, 0.006 */
#define COLOR_BG          ((struct bhs_ui_color){0.006f, 0.006f, 0.008f, 1.0f})

/* Primary Button: Vibrant Royal Blue */
/* sRGB ref: #3b5bdb -> Linear: ~0.045, 0.105, 0.7 */
#define COLOR_PRIMARY     ((struct bhs_ui_color){0.045f, 0.105f, 0.700f, 1.0f})
#define COLOR_PRIMARY_HOVER ((struct bhs_ui_color){0.080f, 0.180f, 0.900f, 1.0f})

/* Secondary Button: Dark Surface */
/* sRGB ref: #2d2d35 -> Linear: ~0.027, 0.027, 0.038 */
#define COLOR_SECONDARY   ((struct bhs_ui_color){0.027f, 0.027f, 0.035f, 1.0f})
#define COLOR_SECONDARY_HOVER ((struct bhs_ui_color){0.045f, 0.045f, 0.055f, 1.0f})

/* Text */
#define COLOR_TEXT        ((struct bhs_ui_color){0.70f, 0.72f, 0.80f, 1.0f}) /* Grey-ish white */
#define COLOR_TEXT_DIM    ((struct bhs_ui_color){0.15f, 0.16f, 0.22f, 1.0f}) /* Dark Grey */
#define COLOR_BORDER      ((struct bhs_ui_color){0.04f, 0.04f, 0.06f, 1.0f})

/* ============================================================================
 * DRAWING HELPERS
 * ============================================================================
 */

/* Simula o icone do Antigravity (curva estilizada) */
/* Function removed: draw_logo_icon (unused) */

static bool custom_button(bhs_ui_ctx_t ctx, const char *label, const char *icon_char, struct bhs_ui_rect rect, bool is_primary)
{
	int32_t mx, my;
	bhs_ui_mouse_pos(ctx, &mx, &my);

	bool hovered = (mx >= rect.x && mx < rect.x + rect.width && 
	                my >= rect.y && my < rect.y + rect.height);
	
	struct bhs_ui_color bg;
	
	if (is_primary) {
		bg = hovered ? COLOR_PRIMARY_HOVER : COLOR_PRIMARY;
		/* No border for primary in Antigravity design */
	} else {
		bg = hovered ? COLOR_SECONDARY_HOVER : COLOR_SECONDARY;
		/* border = COLOR_BORDER; Suppressed unused var, assuming we might use it later or just drop it */
	}

	bhs_ui_draw_rect(ctx, rect, bg);
	if (!is_primary) {
		/* Apenas desenhar borda se secondary (como reference) */
		//bhs_ui_draw_rect_outline(ctx, rect, border, 1.0f);
		bhs_ui_draw_rect_outline(ctx, rect, COLOR_BORDER, 1.0f);
	}
	
	/* Ícone (Fake) */
	float icon_w = 0;
	if (icon_char) {
		/* Desenhar quadrado simbolizando icone de pasta/git */
		bhs_ui_draw_rect_outline(ctx, (struct bhs_ui_rect){rect.x + 15, rect.y + rect.height*0.35f, rect.height*0.3f, rect.height*0.3f}, COLOR_TEXT, 1.0f);
		icon_w = 20.0f;
	}

	if (label) {
		float font = 14.0f; /* Fonte menor e mais elegante (ref: 13-14px) */
		float text_w = bhs_ui_measure_text(ctx, label, font);
		float centerX = rect.x + (rect.width / 2.0f);
		/* if (icon_char) centerX += 10.0f; Unused logic slightly adjusted */
		if (icon_w > 0) centerX += 10.0f;
		
		bhs_ui_draw_text(ctx, label, centerX - (text_w / 2.0f), rect.y + (rect.height - font) / 2.0f, font, COLOR_TEXT);
	}

	return hovered && bhs_ui_mouse_clicked(ctx, 0);
}

static void draw_workspace_item(bhs_ui_ctx_t ctx, float x, float y, float w, float h, 
                               const char *name, const char *path)
{
	struct bhs_ui_rect rect = {x, y, w, h};
	int32_t mx, my;
	bhs_ui_mouse_pos(ctx, &mx, &my);
	bool hovered = (mx >= rect.x && mx < rect.x + rect.width &&
	                my >= rect.y && my < rect.y + rect.height);

	/* Very subtle background for items */
	struct bhs_ui_color bg = hovered ? COLOR_SECONDARY : (struct bhs_ui_color){0.0f, 0.0f, 0.0f, 0.0f};
	
	if (hovered) bhs_ui_draw_rect(ctx, rect, bg);
	bhs_ui_draw_rect_outline(ctx, rect, COLOR_BORDER, 1.0f);
	
	/* Text Alignment */
	float pad_x = 15.0f;
	bhs_ui_draw_text(ctx, name, x + pad_x, y + 12, 14.0f, (struct bhs_ui_color){0.6f, 0.6f, 0.65f, 1.0f});
	bhs_ui_draw_text(ctx, path, x + pad_x, y + 32, 11.0f, COLOR_TEXT_DIM);
}

/* ============================================================================
 * API PRINCIPAL
 * ============================================================================
 */

void bhs_start_screen_draw(struct app_state *app, bhs_ui_ctx_t ctx, int win_w, int win_h)
{
	bhs_ui_clear(ctx, COLOR_BG);

	float cx = (float)win_w / 2.0f;
	float cy = (float)win_h / 2.0f;

	/* ============================================================================
	 * LAYOUT STRUCTURE (Vertical Center)
	 * ============================================================================
	 */
	float container_w = 400.0f; /* Narrower container as per reference */
	
	/* 1. Header (Logo + Title) */
	float header_y = cy - 180.0f;
	
	/* Draw "Antigravity-like" Logo Icon */
	// float icon_size = 48.0f;
	// draw_logo_icon(ctx, cx, header_y, icon_size);
	
	/* Draw Text "Riemann Engine" (instead of Antigravity) */
	/* Small, light grey, centered below logo */
	const char *title = "Riemann Engine";
	float title_size = 16.0f; /* Much smaller than before */
	float title_w = bhs_ui_measure_text(ctx, title, title_size);
	bhs_ui_draw_text(ctx, title, cx - (title_w / 2.0f), header_y + 60.0f, title_size, COLOR_TEXT);

	/* 2. Main Actions */
	float btn_start_y = header_y + 110.0f;
	float btn_h = 42.0f; /* Flatter buttons */
	float gap = 10.0f;
	
	/* Big Blue Button (Primary) */
	struct bhs_ui_rect rect_main = { cx - (container_w / 2.0f), btn_start_y, container_w, btn_h };
	if (custom_button(ctx, "Criar Simulacao", "F", rect_main, true)) {
		scenario_load(app, SCENARIO_EMPTY);
		app->sim_status = APP_SIM_RUNNING;
	}

	/* Secondary Row */
	float sub_w = (container_w - gap) / 2.0f;
	float sub_y = btn_start_y + btn_h + gap;
	
	struct bhs_ui_rect rect_solar = { cx - (container_w / 2.0f), sub_y, sub_w, btn_h };
	if (custom_button(ctx, "Sistema Solar", NULL, rect_solar, false)) {
		scenario_load(app, SCENARIO_SOLAR_SYSTEM);
		app->sim_status = APP_SIM_RUNNING;
	}

	struct bhs_ui_rect rect_tls = { cx + (gap / 2.0f), sub_y, sub_w, btn_h };
	if (custom_button(ctx, "Terra, Lua & Sol", NULL, rect_tls, false)) {
		scenario_load(app, SCENARIO_EARTH_SUN);
		app->sim_status = APP_SIM_RUNNING;
	}

	/* 3. Workspaces List */
	float list_y = sub_y + btn_h + 40.0f;
	
	/* Label "Workspaces" */
	/* Using very dim color for label */
	bhs_ui_draw_text(ctx, "Workspaces", cx - (container_w / 2.0f), list_y, 12.0f, COLOR_TEXT_DIM);
	
	float list_item_h = 52.0f;
	float list_gap = 8.0f;
	float curr_y = list_y + 20.0f;
	
	const char *ws_names[] = {"viagem_interestelar", "orbita_estavel_v1", "colisao_galactica"};
	const char *ws_paths[] = {"~/Simulations/BlackHoles", "~/Simulations/Tests", "/usr/share/riemann/examples"};
	
	/* Draw background box for list? Antigravity has transparent items usually or subtle cards. 
	   We will use outlines as per previous step helper */
	
	for (int i = 0; i < 3; i++) {
		draw_workspace_item(ctx, cx - (container_w / 2.0f), curr_y, container_w, list_item_h, ws_names[i], ws_paths[i]);
		curr_y += list_item_h + list_gap;
	}

	/* Footer */
	const char *more = "Show More...";
	float more_w = bhs_ui_measure_text(ctx, more, 12.0f);
	bhs_ui_draw_text(ctx, more, cx - (more_w / 2.0f), curr_y + 15.0f, 12.0f, COLOR_TEXT_DIM);
}

