/**
 * @file start_screen.c
 * @brief Implementação da Tela de Início "Premium" (Riemann Engine)
 */

#include "start_screen.h"
#include "app_state.h"
#include "simulation/scenario_mgr.h"
#include "gui/ui/lib.h"
#include "gui/ui/layout.h"
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * ESTILO E CORES (VIBE DARK/PREMIUM)
 * ============================================================================
 */

#define COLOR_BG          ((struct bhs_ui_color){0.08f, 0.09f, 0.12f, 1.0f})
#define COLOR_PANEL       ((struct bhs_ui_color){0.14f, 0.16f, 0.22f, 1.0f})
#define COLOR_ACCENT      ((struct bhs_ui_color){0.20f, 0.35f, 0.75f, 1.0f})
#define COLOR_TEXT        ((struct bhs_ui_color){0.95f, 0.95f, 1.00f, 1.0f})
#define COLOR_TEXT_DIM    ((struct bhs_ui_color){0.55f, 0.55f, 0.65f, 1.0f})
#define COLOR_BORDER      ((struct bhs_ui_color){0.20f, 0.22f, 0.30f, 1.0f})

/* ============================================================================
 * DRAWING HELPERS
 * ============================================================================
 */

static void draw_workspace_item(bhs_ui_ctx_t ctx, float x, float y, float w, float h, 
                               const char *name, const char *path)
{
	struct bhs_ui_rect rect = {x, y, w, h};
	bhs_ui_draw_rect(ctx, rect, COLOR_PANEL);
	bhs_ui_draw_rect_outline(ctx, rect, COLOR_BORDER, 1.0f);
	
	bhs_ui_draw_text(ctx, name, x + 15, y + 12, 16.0f, COLOR_TEXT);
	bhs_ui_draw_text(ctx, path, x + 15, y + 36, 12.0f, COLOR_TEXT_DIM);
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

	/* 1. Titulo Central (Riemann Engine) */
	const char *logo = "Riemann Engine";
	float logo_size = (win_h < 600) ? 36.0f : 52.0f;
	float logo_w = bhs_ui_measure_text(ctx, logo, logo_size);
	bhs_ui_draw_text(ctx, logo, cx - (logo_w / 2.0f), cy - (win_h * 0.32f), logo_size, COLOR_TEXT);

	/* 2. Botões Principais */
	float base_w = 600.0f;
	if (base_w > (float)win_w - 60.0f) base_w = (float)win_w - 60.0f;

	float btn_h = 55.0f;
	float gap = 12.0f;
	
	/* Botão 1: Personalizado */
	struct bhs_ui_rect rect_main = { cx - (base_w / 2.0f), cy - 60.0f, base_w, btn_h };
	bhs_ui_draw_rect(ctx, rect_main, COLOR_ACCENT);
	if (bhs_ui_button(ctx, "Criar simulacao Personalizada", rect_main)) {
		scenario_load(app, SCENARIO_EMPTY);
		app->sim_status = APP_SIM_RUNNING;
	}

	/* Botões Secundários */
	float sub_w = (base_w - gap) / 2.0f;
	float sub_h = 48.0f;
	
	struct bhs_ui_rect rect_solar = { cx - (base_w / 2.0f), cy + gap - 60.0f + btn_h, sub_w, sub_h };
	if (bhs_ui_button(ctx, "Sistema Sólar (J2000)", rect_solar)) {
		scenario_load(app, SCENARIO_SOLAR_SYSTEM);
		app->sim_status = APP_SIM_RUNNING;
	}

	struct bhs_ui_rect rect_tls = { cx + (gap / 2.0f), cy + gap - 60.0f + btn_h, sub_w, sub_h };
	if (bhs_ui_button(ctx, "Terra, lua & sol", rect_tls)) {
		scenario_load(app, SCENARIO_EARTH_SUN);
		app->sim_status = APP_SIM_RUNNING;
	}

	/* 4. Workspaces */
	float ws_top = cy + 110.0f;
	if (ws_top + 180.0f > (float)win_h) ws_top = (float)win_h - 220.0f;

	bhs_ui_draw_text(ctx, "Workspaces", cx - (base_w / 2.0f), ws_top, 18.0f, COLOR_TEXT_DIM);
	
	float item_y = ws_top + 32.0f;
	float item_h = 60.0f;
	
	const char *ws_names[] = {"viagem_interestelar", "orbita_estavel_v1", "colisao_galactica"};
	const char *ws_paths[] = {"~/Simulations/BlackHoles", "~/Simulations/Tests", "/usr/share/riemann/examples"};

	for (int i = 0; i < 3; i++) {
		if (item_y + item_h > (float)win_h - 40.0f) break;
		draw_workspace_item(ctx, cx - (base_w / 2.0f), item_y, base_w, item_h, ws_names[i], ws_paths[i]);
		item_y += item_h + gap;
	}

	/* 5. Footer */
	if (item_y < (float)win_h - 20.0f) {
		const char *more = "Show More...";
		float more_w = bhs_ui_measure_text(ctx, more, 14.0f);
		bhs_ui_draw_text(ctx, more, cx - (more_w / 2.0f), item_y + 10.0f, 14.0f, COLOR_TEXT_DIM);
	}
}
