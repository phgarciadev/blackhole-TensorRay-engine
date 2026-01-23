#include "hud.h"
#include <math.h> /* [NEW] for powf */
#include "src/simulation/data/planet.h"
#include "math/units.h" /* [NEW] Para bhs_sim_time_to_date */
#include <stdio.h>
#include "src/simulation/data/orbit_marker.h" /* [NEW] */

static const char *MENU_ITEMS[] = { "Config", "Add", "View", "Refs" };
static const int MENU_COUNT = 4;

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
    /* [FIX] Adicionando dimensões do info panel pra consistência */
    float info_panel_w;
    float info_panel_h;
    float info_panel_margin;
} bhs_ui_layout_t;

static bhs_ui_layout_t get_ui_layout(int win_w, int win_h) {
    bhs_ui_layout_t l;
    /* 
     * [FIX] Base Scale: considera AMBAS dimensões
     * Usa a menor razão pra não distorcer em janelas estreitas/largas
     * (tipo quando tu mete a janela em metade da tela no Hyprland)
     */
    float scale_h = (float)win_h / 1080.0f;
    float scale_w = (float)win_w / 1920.0f;
    l.ui_scale = (scale_h < scale_w) ? scale_h : scale_w;
    if (l.ui_scale < 0.5f) l.ui_scale = 0.5f;  /* Mínimo mais baixo pra janelas pequenas */
    if (l.ui_scale > 2.0f) l.ui_scale = 2.0f;

    /* Metrics */
    l.top_bar_height = 45.0f * l.ui_scale;
    l.padding_x = 15.0f * l.ui_scale;
    l.font_size_logo = 16.0f * l.ui_scale;
    l.font_size_tab = 14.0f * l.ui_scale;

    /* robust "CSS-like" columns */
    /* Logo Column: Fixed minimum width to guarantee space */
    float min_logo_w = 200.0f * l.ui_scale; 
    
    /* Calculate text width heuristic */
    int logo_len = 13; // "RiemannEngine"
    float text_w = logo_len * l.font_size_logo;  /* Fonte monospaced: 1 char = font_size */ 
    
    l.logo_width = (text_w > min_logo_w) ? text_w : min_logo_w;

    /* Layout: [Pad] [Logo] [Pad] [Line] [Pad] [Tabs...] */
    l.tab_start_x = l.padding_x + l.logo_width + (15.0f * l.ui_scale) + (1.0f * l.ui_scale) + (15.0f * l.ui_scale);
    
    /* [FIX] Info Panel - valores centralizados aqui pra draw e input usarem o mesmo */
    l.info_panel_w = 250.0f * l.ui_scale;
    l.info_panel_h = 420.0f * l.ui_scale;
    l.info_panel_margin = 20.0f * l.ui_scale;
    
    return l;
}

void bhs_hud_init(bhs_hud_state_t *state)
{
	if (state) {
		state->show_fps = true;
		state->vsync_enabled = true;
		state->time_scale_val = 0.28f;    /* Default ~1 day/min (Formula: 0.1 * 3650^val) */
		state->active_menu_index = -1;
		state->add_menu_category = -1;
		state->selected_body_index = -1;
		state->req_delete_body = false;
		state->req_add_body_type = -1;
		state->req_add_registry_entry = NULL;
		state->visual_mode = BHS_VISUAL_MODE_DIDACTIC;
		state->top_down_view = false;
		state->show_gravity_line = false;
		state->show_orbit_trail = false;
		state->isolate_view = false;
		state->selected_marker_index = -1;
		state->orbit_history_scroll = 0.0f;
	}
}

void bhs_hud_draw(bhs_ui_ctx_t ctx, bhs_hud_state_t *state, int window_w,
		  int window_h)
{
	(void)window_h;
	
    /* --- LAYOUT CALCULATION --- */
    bhs_ui_layout_t layout = get_ui_layout(window_w, window_h);
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
        float text_w = len * layout.font_size_tab;  /* Fonte monospaced */
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
        /* Calcula dias/min atual */
        float days_per_min = 0.1f * powf(3650.0f, state->time_scale_val);
        
        char telemetry_text[128];
        if (days_per_min < 1.0f) {
            snprintf(telemetry_text, 128, "Speed: %.1f dias/min", days_per_min);
        } else {
            snprintf(telemetry_text, 128, "Speed: %.0f dias/min", days_per_min);
        }
        
        float text_w = 25.0f * (layout.font_size_tab * 0.85f); 
        float margin_right = 20.0f * layout.ui_scale;
        
        bhs_ui_draw_text(ctx, telemetry_text, (float)window_w - text_w - margin_right, 
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
            float w = (len * layout.font_size_tab) + (40.0f * layout.ui_scale);
            dropdown_x += w;
        }
            
		/* Calculate Dynamic Height */
		int count = 0;
		if (state->active_menu_index == 1) {
			const struct bhs_planet_registry_entry *e = bhs_planet_registry_get_head();
			while(e) { count++; e = e->next; }
		} else if (state->active_menu_index == 2) {
			count = 5; /* 3 Modes + TopDown Toggle + Extra space */
		} else {
			count = 4; /* Config: FPS, TimeText, Slider, VSync */
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

		float y = layout.top_bar_height + (15.0f * ui_scale);
		float item_pad = 10.0f * ui_scale;  /* Padding interno do dropdown */
		float item_w = panel_rect.width - (item_pad * 2.0f);  /* Largura dos itens */
		float item_h = 24.0f * ui_scale;  /* Altura dos botoes/checkboxes */
		float font_header = 14.0f * ui_scale;  /* Fonte dos titulos */
		float font_label = 12.0f * ui_scale;   /* Fonte dos labels menores */
		float row_spacing = 28.0f * ui_scale;  /* Espacamento entre linhas */

		/* INDEX 0: CONFIG */
		if (state->active_menu_index == 0) {
			bhs_ui_draw_text(ctx, "Appearance", panel_rect.x + item_pad,
					 y, font_header, BHS_UI_COLOR_GRAY);
			y += 25.0f * ui_scale;

			struct bhs_ui_rect item_rect = { panel_rect.x + item_pad, y,
							 item_w, item_h };
			bhs_ui_checkbox(ctx, "Show FPS", item_rect,
					&state->show_fps);
			
			y += row_spacing;
			item_rect.y = y;

			/* Time Scale Control */
			/* Nova formula: dias terrestres por minuto real */
			/* 0.1 * 3650^val = dias/min (val=0 -> 0.1, val=1 -> 365) */
			float days_per_min = 0.1f * powf(3650.0f, state->time_scale_val);
			char time_label[64];
			if (days_per_min < 1.0f) {
				snprintf(time_label, 64, "Speed: %.1f dias/min", days_per_min);
			} else if (days_per_min < 30.0f) {
				snprintf(time_label, 64, "Speed: %.0f dias/min", days_per_min);
			} else {
				snprintf(time_label, 64, "Speed: %.0f dias/min", days_per_min);
			}
			
			bhs_ui_draw_text(ctx, time_label, panel_rect.x + item_pad, y - 5.0f * ui_scale, font_label, BHS_UI_COLOR_GRAY);
			y += 12.0f * ui_scale;
			item_rect.y = y;
			item_rect.height = item_h * 0.8f;  /* Slider um pouco mais baixo */
			bhs_ui_slider(ctx, item_rect, &state->time_scale_val);

			y += row_spacing;
			item_rect.y = y;
			item_rect.height = item_h;  /* Restaura altura */

			bool vsync_prev = state->vsync_enabled;
			bhs_ui_checkbox(ctx, "Enable VSync", item_rect,
					&state->vsync_enabled);
			if (state->vsync_enabled != vsync_prev) {
				printf("[HUD] VSync changed (Restart "
				       "required)\n");
			}
		}
/* ... inside logic ... */
		/* INDEX 1: ADD */
		else if (state->active_menu_index == 1) {
			bhs_ui_draw_text(ctx, "Inject Body", panel_rect.x + item_pad,
					 y, font_header, BHS_UI_COLOR_GRAY);
			y += 25.0f * ui_scale;

			/* Root Menu: Categories */
			if (state->add_menu_category == -1) {
				const char *categories[] = { "Planets >", "Suns >", "Moons >", "Black Holes >" };
				for (int i = 0; i < 4; i++) {
					struct bhs_ui_rect btn_rect = { panel_rect.x + item_pad, y, item_w, item_h };
					if (bhs_ui_button(ctx, categories[i], btn_rect)) {
						state->add_menu_category = i;
					}
					y += row_spacing;
				}
			}
			/* Sub Menu: Items */
			else {
				/* Back Button */
				struct bhs_ui_rect back_rect = { panel_rect.x + item_pad, y, item_w, item_h };
				if (bhs_ui_button(ctx, "< Back", back_rect)) {
					state->add_menu_category = -1;
				}
				y += row_spacing;

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
						struct bhs_ui_rect btn_rect = { panel_rect.x + item_pad, y, item_w, item_h };
						if (bhs_ui_button(ctx, entry->name, btn_rect)) {
							state->req_add_registry_entry = entry;
							state->req_add_body_type = BHS_BODY_PLANET; /* Simplified for main loop */
							state->active_menu_index = -1; /* Close menu */
							state->add_menu_category = -1; /* Reset */
						}
						y += row_spacing;
					}
					entry = entry->next;
				}
			}
		}
		/* INDEX 2: VIEW (SCALES) */
		else if (state->active_menu_index == 2) {
			bhs_ui_draw_text(ctx, "Visual Scale", panel_rect.x + item_pad,
					 y, font_header, BHS_UI_COLOR_GRAY);
			y += 25.0f * ui_scale;
			
			const char *modes[] = { "Scientific (Real)", "Didactic (Teaching)", "Cinematic (Epic)" };
			bhs_visual_mode_t vals[] = { BHS_VISUAL_MODE_SCIENTIFIC, BHS_VISUAL_MODE_DIDACTIC, BHS_VISUAL_MODE_CINEMATIC };
			
			for (int k = 0; k < 3; k++) {
				bool selected = (state->visual_mode == vals[k]);
				struct bhs_ui_rect btn_rect = { panel_rect.x + item_pad, y, item_w, item_h };
				
				/* Highlight selected */
				if (selected) {
					bhs_ui_draw_rect(ctx, btn_rect, (struct bhs_ui_color){ 0.0f, 0.4f, 0.5f, 0.5f });
				}
				
				if (bhs_ui_button(ctx, modes[k], btn_rect)) {
					state->visual_mode = vals[k];
					/* Auto-close? No, let user switch and see.*/
				}
				y += row_spacing;
			}
			
			/* Top Down Camera Toggle */
			y += 5.0f * ui_scale;
			struct bhs_ui_rect td_rect = { panel_rect.x + item_pad, y, item_w, item_h };
			bhs_ui_checkbox(ctx, "Top Down Camera", td_rect, &state->top_down_view);
			y += row_spacing;

			/* Gravity Line Toggle */
			struct bhs_ui_rect gl_rect = { panel_rect.x + item_pad, y, item_w, item_h };
			bhs_ui_checkbox(ctx, "Gravity Line", gl_rect, &state->show_gravity_line);
			y += row_spacing;

			/* Orbit Trail Toggle */
			struct bhs_ui_rect ot_rect = { panel_rect.x + item_pad, y, item_w, item_h };
			bhs_ui_checkbox(ctx, "Orbit Trail", ot_rect, &state->show_orbit_trail);
			y += row_spacing;
			
			/* Descrição do modo atual */
			y += 5.0f * ui_scale;
			bhs_ui_draw_line(ctx, panel_rect.x + item_pad, y, panel_rect.x + panel_rect.width - item_pad, y, BHS_UI_COLOR_GRAY, 1.0f);
			y += 10.0f * ui_scale;
			
			const char *desc = "";
			switch(state->visual_mode) {
				case BHS_VISUAL_MODE_SCIENTIFIC: desc = "True Physics.\nPlanets are dots.\nSpace is empty."; break;
				case BHS_VISUAL_MODE_DIDACTIC:   desc = "Balanced.\nVisible orbits.\nReadable sizes."; break;
				case BHS_VISUAL_MODE_CINEMATIC:  desc = "Hollywood.\nMassive planets.\nClose Stars.\nNot physics."; break;
			}
			bhs_ui_draw_text(ctx, desc, panel_rect.x + item_pad, y, font_label, BHS_UI_COLOR_GRAY);
		}
		/* INDEX 3: REFS (References - Temporal) */
		else if (state->active_menu_index == 3) {
			bhs_ui_draw_text(ctx, "Reference Frame", panel_rect.x + item_pad,
					 y, font_header, BHS_UI_COLOR_GRAY);
			y += 25.0f * ui_scale;

			/* Epoch Info */
			bhs_ui_draw_text(ctx, "Epoch: J2000.0", panel_rect.x + item_pad, y, 
					 font_label, (struct bhs_ui_color){0.6f, 0.6f, 0.6f, 1.0f});
			y += 18.0f * ui_scale;
			bhs_ui_draw_text(ctx, "Start: 2000-01-01 12:00 UTC", panel_rect.x + item_pad, y, 
					 font_label, (struct bhs_ui_color){0.5f, 0.5f, 0.5f, 1.0f});
			y += 25.0f * ui_scale;

			/* Separador */
			bhs_ui_draw_line(ctx, panel_rect.x + item_pad, y, 
					 panel_rect.x + panel_rect.width - item_pad, y, 
					 BHS_UI_COLOR_GRAY, 1.0f);
			y += 15.0f * ui_scale;

			/* Current Date Header */
			bhs_ui_draw_text(ctx, "CURRENT DATE (UTC)", panel_rect.x + item_pad, y, 
					 font_header, (struct bhs_ui_color){0.0f, 0.8f, 1.0f, 1.0f});
			y += 25.0f * ui_scale;

			/* Converte segundos desde J2000 para data */
			int yr, mo, dy, hr, mi, sc;
			bhs_sim_time_to_date(state->sim_time_seconds, &yr, &mo, &dy, &hr, &mi, &sc);

			/* Data formatada */
			char date_buf[64];
			snprintf(date_buf, sizeof(date_buf), "%04d-%02d-%02d", yr, mo, dy);
			bhs_ui_draw_text(ctx, date_buf, panel_rect.x + item_pad, y, 
					 20.0f * ui_scale, BHS_UI_COLOR_WHITE);
			y += 28.0f * ui_scale;

			/* Hora formatada */
			char time_buf[64];
			snprintf(time_buf, sizeof(time_buf), "%02d:%02d:%02d UTC", hr, mi, sc);
			bhs_ui_draw_text(ctx, time_buf, panel_rect.x + item_pad, y, 
					 16.0f * ui_scale, (struct bhs_ui_color){0.8f, 0.8f, 0.8f, 1.0f});
			y += 30.0f * ui_scale;

			/* Tempo decorrido */
			double days_elapsed = state->sim_time_seconds / 86400.0;
			double years_elapsed = days_elapsed / 365.25;
			char elapsed_buf[64];
			if (years_elapsed >= 1.0) {
				snprintf(elapsed_buf, sizeof(elapsed_buf), "%.2f years since J2000", years_elapsed);
			} else {
				snprintf(elapsed_buf, sizeof(elapsed_buf), "%.1f days since J2000", days_elapsed);
			}
			bhs_ui_draw_text(ctx, elapsed_buf, panel_rect.x + item_pad, y, 
					 font_label, (struct bhs_ui_color){0.5f, 0.5f, 0.5f, 1.0f});
		}
	}

	/* 4. Info Panel (Expanded & Polished) */
	if (state->selected_body_index != -1 || state->selected_marker_index != -1) {
		/* [FIX] Usando valores do layout pra consistência com hit test */
		struct bhs_ui_rect info_rect = {
			(float)window_w - layout.info_panel_w - layout.info_panel_margin,
			layout.top_bar_height + 10.0f * ui_scale,
			layout.info_panel_w,
			layout.info_panel_h
		};
        /* Sleek Dark Panel */
        bhs_ui_panel(ctx, info_rect,
			     (struct bhs_ui_color){ 0.05f, 0.05f, 0.05f, 0.95f },
			     (struct bhs_ui_color){ 0.2f, 0.2f, 0.2f, 1.0f });

		float pad = 15.0f * ui_scale;  /* Padding interno */
		float y = info_rect.y + pad;
		float x = info_rect.x + pad;
        float w = info_rect.width - (pad * 2.0f);
		
		/* Font sizes escalados */
		float font_title = 14.0f * ui_scale;
		float font_name = 16.0f * ui_scale;
		float font_prop = 13.0f * ui_scale;
		float font_section = 12.0f * ui_scale;
		float prop_offset = 80.0f * ui_scale;  /* Offset pra valor das props */
		float line_h = 18.0f * ui_scale;  /* Altura de linha */

		/* Macro para desenhar propriedades (reutilizável) */
		char buf[128];
		#undef DRAW_PROP
		#define DRAW_PROP(label, val_fmt, ...) \
			do { \
				bhs_ui_draw_text(ctx, label, x, y, font_prop, (struct bhs_ui_color){0.6f, 0.6f, 0.6f, 1.0f}); \
				snprintf(buf, sizeof(buf), val_fmt, __VA_ARGS__); \
				bhs_ui_draw_text(ctx, buf, x + prop_offset, y, font_prop, BHS_UI_COLOR_WHITE); \
				y += line_h; \
			} while(0)

		/* CASO A: Marcador de Órbita Selecionado */
		if (state->selected_marker_index != -1 && state->orbit_markers_ptr) {
			const struct bhs_orbit_marker *m = &state->orbit_markers_ptr->markers[state->selected_marker_index];
			
			/* Header */
			bhs_ui_draw_text(ctx, "ORBIT EVENT", x, y, font_title, (struct bhs_ui_color){0.6f, 0.2f, 0.8f, 1.0f});
			y += 20.0f * ui_scale;
			bhs_ui_draw_line(ctx, x, y, x + w, y, (struct bhs_ui_color){0.6f, 0.2f, 0.8f, 0.3f}, 1.0f * ui_scale);
			y += 15.0f * ui_scale;

			/* Nome (Planeta que completou) */
			bhs_ui_draw_text(ctx, m->planet_name, x, y, font_name, BHS_UI_COLOR_WHITE);
			y += 25.0f * ui_scale;

			/* Detalhes do Marco */
			DRAW_PROP("Orbit #:", "%d", m->orbit_number);
			
			int yr, mo, dy, hr, mi, sc;
			bhs_sim_time_to_date(m->timestamp_seconds, &yr, &mo, &dy, &hr, &mi, &sc);
			DRAW_PROP("Date:", "%04d-%02d-%02d", yr, mo, dy);
			DRAW_PROP("Time:", "%02d:%02d:%02d UTC", hr, mi, sc);
			
			y += 10.0f * ui_scale;
			bhs_ui_draw_text(ctx, "MEASUREMENTS", x, y, font_section, (struct bhs_ui_color){0.6f, 0.2f, 0.8f, 0.8f});
			y += line_h;

			DRAW_PROP("Period:", "%.3f days", m->orbital_period_measured / 86400.0);
			DRAW_PROP("Pos X:", "%.2e m", m->position.x);
			DRAW_PROP("Pos Z:", "%.2e m", m->position.z);
			
			
			/* Seção de HISTÓRICO */
			y += 20.0f * ui_scale;
			bhs_ui_draw_line(ctx, x, y, x + w, y, (struct bhs_ui_color){0.3f, 0.3f, 0.3f, 0.8f}, 1.0f * ui_scale);
			y += 10.0f * ui_scale;
			bhs_ui_draw_text(ctx, "ORBIT HISTORY", x, y, font_section, (struct bhs_ui_color){0.6f, 0.2f, 0.8f, 0.8f});
			y += line_h + 5.0f * ui_scale;

			/* Área rolável */
			float history_y_start = y;
			float history_h_max = info_rect.y + info_rect.height - y - 10.0f * ui_scale;
			
			/* Lógica de Scroll do mouse */
			int mx, my;
			bhs_ui_mouse_pos(ctx, &mx, &my);
			if (is_inside(mx, my, info_rect.x, history_y_start, info_rect.width, history_h_max)) {
				float scroll = bhs_ui_mouse_scroll(ctx);
				if (fabsf(scroll) > 0.1f) {
					state->orbit_history_scroll += scroll * 20.0f * ui_scale;
				}
			}

			/* Iterar todos os marcadores do mesmo planeta */
			int count_found = 0;
			if (state->orbit_markers_ptr) {
				for (int i = 0; i < state->orbit_markers_ptr->marker_count; i++) {
					const struct bhs_orbit_marker *hist = &state->orbit_markers_ptr->markers[i];
					if (!hist->active || hist->planet_index != m->planet_index) continue;

					/* Calcula Y com scroll */
					float entry_y = y + state->orbit_history_scroll;
					
					/* Clipping simples */
					if (entry_y >= history_y_start && entry_y < history_y_start + history_h_max - 15.0f * ui_scale) {
						int hyr, hmo, hdy, hhr, hmi, hsc;
						bhs_sim_time_to_date(hist->timestamp_seconds, &hyr, &hmo, &hdy, &hhr, &hmi, &hsc);
						
						char entry_buf[128];
						snprintf(entry_buf, sizeof(entry_buf), "#%d | %04d-%02d-%02d | %.1f d", 
							 hist->orbit_number, hyr, hmo, hdy, hist->orbital_period_measured / 86400.0);
						
						bhs_ui_draw_text(ctx, entry_buf, x, entry_y, font_section * 0.9f, 
								 (hist->orbit_number == m->orbit_number) ? BHS_UI_COLOR_WHITE : (struct bhs_ui_color){0.6f, 0.6f, 0.61f, 1.0f});
					}
					
					y += 15.0f * ui_scale; /* Mantém o y "virtual" crescendo */
					count_found++;
				}
			}

			/* Clamping do scroll */
			float total_content_h = count_found * 15.0f * ui_scale;
			float min_scroll = -(total_content_h - history_h_max);
			if (min_scroll > 0) min_scroll = 0;
			if (state->orbit_history_scroll < min_scroll) state->orbit_history_scroll = min_scroll;
			if (state->orbit_history_scroll > 0) state->orbit_history_scroll = 0;

			return; /* Sai aqui se for marcador */
		}

		/* CASO B: Corpo Celeste Selecionado (Código original migrado para dentro do if) */
		const struct bhs_body *b = &state->selected_body_cache;

		/* Header */
		bhs_ui_draw_text(ctx, "OBJECT INSPECTOR", x, y, font_title, (struct bhs_ui_color){0.0f, 0.8f, 1.0f, 1.0f});
		y += 20.0f * ui_scale;
        
        /* High-tech Separator Line */
        bhs_ui_draw_line(ctx, x, y, x + w, y, (struct bhs_ui_color){0.0f, 0.8f, 1.0f, 0.3f}, 1.0f * ui_scale);
		y += 15.0f * ui_scale;

		/* --- UNIVERSAL DATA --- */
		const char* type_str = "Unknown";
		if (b->name[0] != '\0') type_str = b->name;
		
        /* Name (Large) */
		bhs_ui_draw_text(ctx, type_str, x, y, font_name, BHS_UI_COLOR_WHITE);
		y += 25.0f * ui_scale;

        /* Helper to draw property row - agora usando variaveis escaladas */
        #undef DRAW_PROP
        #define DRAW_PROP(label, val_fmt, ...) \
            do { \
                bhs_ui_draw_text(ctx, label, x, y, font_prop, (struct bhs_ui_color){0.6f, 0.6f, 0.6f, 1.0f}); \
                snprintf(buf, sizeof(buf), val_fmt, __VA_ARGS__); \
                bhs_ui_draw_text(ctx, buf, x + prop_offset, y, font_prop, BHS_UI_COLOR_WHITE); \
                y += line_h; \
            } while(0)

        // Basic Physics
		DRAW_PROP("Mass:", "%.2e kg", b->state.mass);
		DRAW_PROP("Radius:", "%.2e m", b->state.radius);
		DRAW_PROP("Pos:", "(%.1f, %.1f)", b->state.pos.x, b->state.pos.z);
		DRAW_PROP("Vel:", "(%.2f, %.2f)", b->state.vel.x, b->state.vel.z);
		y += 10.0f * ui_scale;

        /* Separator */
        bhs_ui_draw_line(ctx, x, y, x + w, y, (struct bhs_ui_color){0.3f, 0.3f, 0.3f, 1.0f}, 1.0f * ui_scale);
        y += 10.0f * ui_scale;
        
        bhs_ui_draw_text(ctx, "PROPERTIES", x, y, font_section, (struct bhs_ui_color){0.0f, 0.8f, 1.0f, 0.8f});
		y += line_h;

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
		
        /* Strongest Attractor Display */
        if (state->attractor_name[0] != '\0') {
            y += 10.0f * ui_scale;
            bhs_ui_draw_line(ctx, x, y, x + w, y, (struct bhs_ui_color){0.3f, 0.3f, 0.3f, 1.0f}, 1.0f * ui_scale);
            y += 10.0f * ui_scale;

            bhs_ui_draw_text(ctx, "MAJOR FORCE", x, y, font_section, (struct bhs_ui_color){0.0f, 0.8f, 1.0f, 0.8f});
            y += line_h;

            DRAW_PROP("Source:", "%s", state->attractor_name);
            
            double d_val = state->attractor_dist;
            /* Smart Units: AU > Mkm > km */
            if (d_val >= 149597870700.0 * 0.01) { /* > 0.01 AU (~1.5 Mkm) */
                DRAW_PROP("Dist:", "%.3f AU", d_val / 149597870700.0);
            } else if (d_val >= 1.0e9) { /* > 1 Mkm */
                DRAW_PROP("Dist:", "%.2f Mkm", d_val / 1.0e9);
            } else {
                 DRAW_PROP("Dist:", "%.0f km", d_val / 1000.0);
            }
        }

		y += 20.0f * ui_scale; /* Spacing before buttons */

		/* Delete Button (Styled) */
		float btn_h = 28.0f * ui_scale;
		struct bhs_ui_rect del_rect = { x, y, w, btn_h };
        /* Background */
		bhs_ui_draw_rect(ctx, del_rect, (struct bhs_ui_color){ 0.4f, 0.1f, 0.1f, 1.0f });
        /* Border */
        bhs_ui_draw_rect_outline(ctx, del_rect, (struct bhs_ui_color){ 0.8f, 0.2f, 0.2f, 1.0f }, 1.0f * ui_scale);
        
        /* Centered Text */
		if (bhs_ui_button(ctx, "", del_rect)) {
			state->req_delete_body = true;
		}
		float text_w = 11.0f * (font_prop * 0.65f);  /* Aprox largura de "DELETE BODY" */
        bhs_ui_draw_text(ctx, "DELETE BODY", x + (w - text_w) / 2.0f, y + btn_h * 0.25f, font_prop, BHS_UI_COLOR_WHITE);

		/* [NEW] Checkbox para Isolamento Visual */
		y += btn_h + 10.0f * ui_scale;
		struct bhs_ui_rect iso_rect = { x, y, w, 24.0f * ui_scale };
		bhs_ui_checkbox(ctx, "Isolar Visao", iso_rect, &state->isolate_view);
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
    bhs_ui_layout_t layout = get_ui_layout(win_w, win_h);

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
            float w = (len * layout.font_size_tab) + (40.0f * layout.ui_scale);
            dropdown_x += w;
        }

		if (is_inside(mx, my, dropdown_x, layout.top_bar_height, 
                      200.0f * layout.ui_scale, 600.0f * layout.ui_scale))
			return true;
	}

	/* 3. Info Panel - [FIX] usando mesmos valores do draw */
	if (state->selected_body_index != -1) {
		float panel_x = (float)win_w - layout.info_panel_w - layout.info_panel_margin;
		float panel_y = layout.top_bar_height + 10.0f * layout.ui_scale;
		if (is_inside(mx, my, panel_x, panel_y, 
			      layout.info_panel_w, layout.info_panel_h))
			return true;
	}

	return false;
}

