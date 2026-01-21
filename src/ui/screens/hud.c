#include "hud.h"
#include <math.h> /* [NEW] for powf */
#include "src/simulation/data/planet.h"
#include <stdio.h>

static const char *MENU_ITEMS[] = { "Config", "Add" };
static const int MENU_COUNT = 2;

static bool is_inside(int mx, int my, float x, float y, float w, float h);

/* Layout helper struct to ensure consistency between Draw and Input */
typedef struct {
    float ui_scale;
    float top_bar_height;
    float padding_x;
    float font_size_logo;
    float font_size_tab;
    float logo_width;
    float tab_start_x;
} bhs_ui_layout_t;

static bhs_ui_layout_t get_ui_layout(int win_h) {
    bhs_ui_layout_t l;
    /* Base Scale */
    l.ui_scale = (float)win_h / 1080.0f;
    if (l.ui_scale < 0.8f) l.ui_scale = 0.8f;
    if (l.ui_scale > 2.0f) l.ui_scale = 2.0f;

    /* Metrics */
    l.top_bar_height = 45.0f * l.ui_scale;
    l.padding_x = 15.0f * l.ui_scale;
    l.font_size_logo = 16.0f * l.ui_scale;
    l.font_size_tab = 14.0f * l.ui_scale;

    /* robust "CSS-like" columns */
    /* Logo Column: Fixed minimum width to guarantee space */
    /* Logo Column: Fixed minimum width to guarantee space */
    /* [ADJUSTED] Aumentei o mínimo pra 200 pra garantir que o separador não fique grudado */
    float min_logo_w = 200.0f * l.ui_scale; 
    
    /* Calculate text width heuristic */
    int logo_len = 13; // "RiemannEngine"
    /* [ADJUSTED] O fator 0.7 era muito otimista. 0.9 é mais seguro (melhor sobrar que faltar) */
    float text_w = logo_len * (l.font_size_logo * 0.9f); 
    
    l.logo_width = (text_w > min_logo_w) ? text_w : min_logo_w;

    /* Layout: [Pad] [Logo] [Pad] [Line] [Pad] [Tabs...] */
    l.tab_start_x = l.padding_x + l.logo_width + (15.0f * l.ui_scale) + (1.0f * l.ui_scale) + (15.0f * l.ui_scale);
    
    return l;
}

void bhs_hud_init(bhs_hud_state_t *state)
{
	if (state) {
		state->show_fps = true;
		state->vsync_enabled = true;
		state->time_scale_val = 0.5f;      /* Default Time Scale (1.0x mapped from 0.5) */
		state->active_menu_index = -1;
		state->add_menu_category = -1;
		state->selected_body_index = -1;
		state->req_delete_body = false;
		state->req_add_body_type = -1;
		state->req_add_registry_entry = NULL;
	}
}

void bhs_hud_draw(bhs_ui_ctx_t ctx, bhs_hud_state_t *state, int window_w,
		  int window_h)
{
	(void)window_h;
	
    /* --- LAYOUT CALCULATION --- */
    bhs_ui_layout_t layout = get_ui_layout(window_h);
    float ui_scale = layout.ui_scale; /* Keep for local usage if needed */

    struct bhs_ui_color theme_bg = { 0.05f, 0.05f, 0.05f, 0.95f };
    struct bhs_ui_color theme_border = { 0.0f, 0.8f, 1.0f, 0.3f };
    struct bhs_ui_color theme_text_normal = { 0.7f, 0.7f, 0.7f, 1.0f };
    struct bhs_ui_color theme_text_active = { 1.0f, 1.0f, 1.0f, 1.0f };
    struct bhs_ui_color theme_highlight = { 0.0f, 0.8f, 1.0f, 1.0f };

	/* 1. Header Bar Background */
	struct bhs_ui_rect bar_rect = {
		.x = 0, .y = 0, .width = (float)window_w, .height = layout.top_bar_height
	};
	bhs_ui_draw_rect(ctx, bar_rect, theme_bg);
    bhs_ui_draw_line(ctx, 0, layout.top_bar_height, (float)window_w, layout.top_bar_height, theme_border, 1.0f * ui_scale);

	/* 2. Branding / Logo (Fixed Column) */
	float x_cursor = layout.padding_x;
    const char* logo_text = "RiemannEngine";
    /* Vertically Center: (BarH - FontH) / 2 */
    float logo_y = (layout.top_bar_height - layout.font_size_logo) * 0.5f;
    bhs_ui_draw_text(ctx, logo_text, x_cursor, logo_y, layout.font_size_logo, (struct bhs_ui_color){ 1.0f, 1.0f, 1.0f, 1.0f });
    
    /* Move cursor to end of logo column */
    x_cursor += layout.logo_width;
    
    /* Divider */
    float div_x = x_cursor + (15.0f * ui_scale);
    float div_h = 20.0f * ui_scale;
    float div_y = (layout.top_bar_height - div_h) * 0.5f;
    bhs_ui_draw_line(ctx, div_x, div_y, div_x, div_y + div_h, (struct bhs_ui_color){0.4f, 0.4f, 0.4f, 0.8f}, 1.0f * ui_scale);
    
    /* 3. Navigation Tabs */
    /* Start exactly where the layout says, robustly */
    x_cursor = layout.tab_start_x;
    
	for (int i = 0; i < MENU_COUNT; i++) {
        bool is_active = (state->active_menu_index == i);
        
		int len = 0;
		while (MENU_ITEMS[i][len] != '\0') len++;

        /* Tab Width: Text + Padding (CSS: padding: 0 20px) */
        float item_padding = 20.0f * ui_scale;
        float text_w = len * (layout.font_size_tab * 0.65f);
		float width = text_w + (item_padding * 2.0f);
        
		struct bhs_ui_rect item_rect = { x_cursor, 0, width, layout.top_bar_height };
        
        /* Input Update */
        int mx, my;
        bhs_ui_mouse_pos(ctx, &mx, &my);
        bool hovered = is_inside(mx, my, item_rect.x, item_rect.y, item_rect.width, item_rect.height);
        
        if (hovered) {
             bhs_ui_draw_rect(ctx, item_rect, (struct bhs_ui_color){ 1.0f, 1.0f, 1.0f, 0.05f });
             if (bhs_ui_mouse_clicked(ctx, 0)) {
                 state->active_menu_index = (state->active_menu_index == i) ? -1 : i;
             }
        }

        /* Render Text Centered */
        float text_y = (layout.top_bar_height - layout.font_size_tab) * 0.5f;
        float text_x = x_cursor + item_padding;
        
        bhs_ui_draw_text(ctx, MENU_ITEMS[i], text_x, text_y, layout.font_size_tab, 
                         is_active ? theme_text_active : (hovered ? theme_text_active : theme_text_normal));
                         
        /* Active Indicator */
        if (is_active) {
            float line_y = layout.top_bar_height - (2.0f * ui_scale);
            bhs_ui_draw_line(ctx, x_cursor + (5.0f * ui_scale), line_y, 
                             x_cursor + width - (5.0f * ui_scale), line_y, 
                             theme_highlight, 2.0f * ui_scale);
        }

		x_cursor += width;
	}
    
    /* 4. Telemetry (Top Right) */
    if (state->show_fps) {
        char fps_text[64]; /* Increased buffer size for safety */
        /* Assume fixed 60 for now per original code, or fetch from engine if available later */
		snprintf(fps_text, 64, "FPS: 60 | T=%.1fs", 124.5); /* Placeholder time */ 
        /* Right aligned with proper dynamic margin */
        /* width approx: 20 chars * font_size * 0.6 */
        /* width approx: 20 chars * font_size * 0.6 */
        
        /* [FIX] O cálculo anterior (22.0 * 0.6) dava uma largura de ~13u * scale, o que era pouco. 
         * O texto real tem uns 20 chars. 
         * Ajustando fator para 0.85f (fonte monospace aproximada) se não corta o final.
         * E usando strlen real (aprox 25 chars margem segurança).
         */
        float fps_w = 25.0f * (layout.font_size_tab * 0.85f); 
        float margin_right = 20.0f * layout.ui_scale;
        
        bhs_ui_draw_text(ctx, fps_text, (float)window_w - fps_w - margin_right, 
                         17.0f * layout.ui_scale, layout.font_size_tab, theme_text_normal);
    }

	/* 3. Dropdown Menu */
	if (state->active_menu_index != -1) {
        /* Use pre-calculated consistent start position */
        float dropdown_x = layout.tab_start_x;
        
        /* Calculate offset to active item */
        for(int j=0; j<state->active_menu_index; ++j) {
            int len = 0; while (MENU_ITEMS[j][len] != '\0') len++;
             /* Same width calc as Draw */
            float w = (len * (layout.font_size_tab * 0.65f)) + (40.0f * layout.ui_scale);
            dropdown_x += w;
        }
            
		/* Calculate Dynamic Height */
		int count = 0;
		if (state->active_menu_index == 1) {
			const struct bhs_planet_registry_entry *e = bhs_planet_registry_get_head();
			while(e) { count++; e = e->next; }
		} else {
			count = 2; // Config menu items
		}
		
		float row_h = 28.0f * layout.ui_scale;
		float panel_h = (40.0f * layout.ui_scale) + (count * row_h);
		if (panel_h < 150.0f * layout.ui_scale) panel_h = 150.0f * layout.ui_scale;

		struct bhs_ui_rect panel_rect = { dropdown_x, layout.top_bar_height, 200.0f * layout.ui_scale, panel_h };

		/* Modern Menu Bg */
		bhs_ui_panel(
			ctx, panel_rect,
			(struct bhs_ui_color){ 0.1f, 0.1f, 0.1f, 0.98f },
			theme_border); /* Border */

		float y = layout.top_bar_height + (15.0f * layout.ui_scale);

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

			/* Time Scale Control */
			/* Mapping: 0.0->0.1x, 0.5->1.0x, 1.0->10.0x (Logarithmic feel) */
			/* Formula: 0.1 * 100^val */
			float time_scale_disp = 0.1f * powf(100.0f, state->time_scale_val);
			char time_label[64];
			snprintf(time_label, 64, "Time Speed: %.2fx", time_scale_disp);
			
			bhs_ui_draw_text(ctx, time_label, panel_rect.x + 10, y - 5, 12.0f, BHS_UI_COLOR_GRAY);
			y += 12.0f;
			item_rect.y = y;
			bhs_ui_slider(ctx, item_rect, &state->time_scale_val);

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

			/* Root Menu: Categories */
			if (state->add_menu_category == -1) {
				const char *categories[] = { "Planets >", "Suns >", "Moons >", "Black Holes >" };
				for (int i = 0; i < 4; i++) {
					struct bhs_ui_rect btn_rect = { panel_rect.x + 10, y, 180, 24 };
					if (bhs_ui_button(ctx, categories[i], btn_rect)) {
						state->add_menu_category = i;
					}
					y += 28.0f;
				}
			}
			/* Sub Menu: Items */
			else {
				/* Back Button */
				struct bhs_ui_rect back_rect = { panel_rect.x + 10, y, 180, 24 };
				if (bhs_ui_button(ctx, "< Back", back_rect)) {
					state->add_menu_category = -1;
				}
				y += 28.0f;

				/* Iterate Registry and Filter */
				const struct bhs_planet_registry_entry *entry = bhs_planet_registry_get_head();
				while (entry) {
					/* Filter by Category */
					bool show = false;
					if (entry->getter) {
						struct bhs_planet_desc d = entry->getter();
						switch (state->add_menu_category) {
						case 0: /* Planets */
							show = (d.type == BHS_PLANET_TERRESTRIAL || 
								d.type == BHS_PLANET_GAS_GIANT || 
								d.type == BHS_PLANET_ICE_GIANT || 
								d.type == BHS_PLANET_DWARF);
							break;
						case 1: /* Suns */
							show = (d.type == BHS_STAR_MAIN_SEQ);
							break;
						case 2: /* Moons */
							/* No explicit moon type in detailed enum yet, assuming none match or explicit if added */
							show = false;
							break;
						case 3: /* BlackHoles */
							show = (d.type == BHS_BLACK_HOLE);
							break;
						}
					}

					if (show) {
						struct bhs_ui_rect btn_rect = { panel_rect.x + 10, y, 180, 24 };
						if (bhs_ui_button(ctx, entry->name, btn_rect)) {
							state->req_add_registry_entry = entry;
							state->req_add_body_type = BHS_BODY_PLANET; /* Simplified for main loop */
							state->active_menu_index = -1; /* Close menu */
							state->add_menu_category = -1; /* Reset */
						}
						y += 28.0f;
					}
					entry = entry->next;
				}
			}
		}
	}

	/* 4. Info Panel (Expanded & Polished) */
	if (state->selected_body_index != -1) {
		/* Aumentar altura do painel para caber dados */
		struct bhs_ui_rect info_rect = { (float)window_w - 270, 50, 250, 420 };
		
        /* Sleek Dark Panel */
        bhs_ui_panel(ctx, info_rect,
			     (struct bhs_ui_color){ 0.05f, 0.05f, 0.05f, 0.95f },
			     (struct bhs_ui_color){ 0.2f, 0.2f, 0.2f, 1.0f });

		float y = info_rect.y + 15.0f;
		float x = info_rect.x + 15.0f;
        float w = info_rect.width - 30.0f;
		const struct bhs_body *b = &state->selected_body_cache;

		/* Header */
		bhs_ui_draw_text(ctx, "OBJECT INSPECTOR", x, y, 14.0f, (struct bhs_ui_color){0.0f, 0.8f, 1.0f, 1.0f}); // Cyan Header
		y += 20.0f;
        
        /* High-tech Separator Line */
        bhs_ui_draw_line(ctx, x, y, x + w, y, (struct bhs_ui_color){0.0f, 0.8f, 1.0f, 0.3f}, 1.0f);
		y += 15.0f;

		/* --- UNIVERSAL DATA --- */
		char buf[128];
		
		const char* type_str = "Unknown";
		if (b->name[0] != '\0') type_str = b->name;
		
        /* Name (Large) */
		bhs_ui_draw_text(ctx, type_str, x, y, 16.0f, BHS_UI_COLOR_WHITE);
		y += 25.0f;

        /* Helper to draw property row */
        #define DRAW_PROP(label, val_fmt, ...) \
            do { \
                bhs_ui_draw_text(ctx, label, x, y, 13.0f, (struct bhs_ui_color){0.6f, 0.6f, 0.6f, 1.0f}); \
                snprintf(buf, sizeof(buf), val_fmt, __VA_ARGS__); \
                /* Rough right align? No, stick to left with offset for now, cleaner code */ \
                bhs_ui_draw_text(ctx, buf, x + 80, y, 13.0f, BHS_UI_COLOR_WHITE); \
                y += 18.0f; \
            } while(0)

        // Basic Physics
		DRAW_PROP("Mass:", "%.2e kg", b->state.mass);
		DRAW_PROP("Radius:", "%.2e m", b->state.radius);
		DRAW_PROP("Pos:", "(%.1f, %.1f)", b->state.pos.x, b->state.pos.z);
		DRAW_PROP("Vel:", "(%.2f, %.2f)", b->state.vel.x, b->state.vel.z);
		y += 10.0f;

        /* Separator */
        bhs_ui_draw_line(ctx, x, y, x + w, y, (struct bhs_ui_color){0.3f, 0.3f, 0.3f, 1.0f}, 1.0f);
        y += 10.0f;
        
        bhs_ui_draw_text(ctx, "PROPERTIES", x, y, 12.0f, (struct bhs_ui_color){0.0f, 0.8f, 1.0f, 0.8f});
		y += 18.0f;

		/* --- TYPE SPECIFIC --- */
		if (b->type == BHS_BODY_PLANET) {
            DRAW_PROP("Density:", "%.0f kg/m3", b->prop.planet.density);
            DRAW_PROP("Temp:", "%.1f K", b->prop.planet.temperature);
            DRAW_PROP("Press:", "%.2f atm", b->prop.planet.surface_pressure);
            DRAW_PROP("Atmos:", "%s", b->prop.planet.has_atmosphere ? "Yes" : "No");
            DRAW_PROP("Comp:", "%s", b->prop.planet.composition);
		}
		else if (b->type == BHS_BODY_STAR) {
            DRAW_PROP("Lum:", "%.2e W", b->prop.star.luminosity);
            DRAW_PROP("Teff:", "%.0f K", b->prop.star.temp_effective);
            DRAW_PROP("Class:", "%s", b->prop.star.spectral_type);
            DRAW_PROP("Age:", "%.1e yr", b->prop.star.age);
		}
		else if (b->type == BHS_BODY_BLACKHOLE) {
            DRAW_PROP("Spin:", "%.2f", b->prop.bh.spin_factor);
            DRAW_PROP("Horizon:", "%.2f", b->prop.bh.event_horizon_r);
		}
		
        /* [NEW] Strongest Attractor Display */
        if (state->attractor_name[0] != '\0') {
            y += 10.0f;
            bhs_ui_draw_line(ctx, x, y, x + w, y, (struct bhs_ui_color){0.3f, 0.3f, 0.3f, 1.0f}, 1.0f);
            y += 10.0f;

            bhs_ui_draw_text(ctx, "MAJOR FORCE", x, y, 12.0f, (struct bhs_ui_color){0.0f, 0.8f, 1.0f, 0.8f});
            y += 18.0f;

            DRAW_PROP("Source:", "%s", state->attractor_name);
            DRAW_PROP("Dist:", "%.2e m", state->attractor_dist);
        }

		y += 20.0f; /* Spacing before buttons */

		/* Delete Button (Styled) */
		struct bhs_ui_rect del_rect = { x, y, w, 28 };
        /* Background */
		bhs_ui_draw_rect(ctx, del_rect, (struct bhs_ui_color){ 0.4f, 0.1f, 0.1f, 1.0f });
        /* Border */
        bhs_ui_draw_rect_outline(ctx, del_rect, (struct bhs_ui_color){ 0.8f, 0.2f, 0.2f, 1.0f }, 1.0f);
        
        /* Centered Text */
        /* Hardcoded Approx Center: Rect Width ~220, Text "DELETE" ~40 -> x+90 */
		if (bhs_ui_button(ctx, "", del_rect)) { // Empty label to handle click, manual draw text
			state->req_delete_body = true;
		}
        bhs_ui_draw_text(ctx, "DELETE BODY", x + (w/2) - 40, y + 6, 13.0f, BHS_UI_COLOR_WHITE);
	}


	/* 4. FPS Counter (Overlay - independent of menu) */
	/* FPS Counter moved to Top Bar */
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
	(void)win_h;
    
    bhs_ui_layout_t layout = get_ui_layout(win_h);
    /* No manual scale calc needed */

	/* 1. Top Bar */
	if (is_inside(mx, my, 0, 0, (float)win_w, layout.top_bar_height))
		return true;

	/* 2. Dropdown Menu */
	if (state->active_menu_index != -1) {
         float dropdown_x = layout.tab_start_x;
         
        /* Calculate offset to active item */
        for(int j=0; j<state->active_menu_index; ++j) {
            int len = 0; while (MENU_ITEMS[j][len] != '\0') len++;
             /* Same width calc as Draw */
            float w = (len * (layout.font_size_tab * 0.65f)) + (40.0f * layout.ui_scale);
            dropdown_x += w;
        }

		if (is_inside(mx, my, dropdown_x, layout.top_bar_height, 
                      200.0f * layout.ui_scale, 600.0f * layout.ui_scale))
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
