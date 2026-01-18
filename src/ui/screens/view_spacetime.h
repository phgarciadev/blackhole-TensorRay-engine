#ifndef BHS_UX_UI_VIEW_SPACETIME_H
#define BHS_UX_UI_VIEW_SPACETIME_H

struct bhs_fabric; /* Forward Declaration */

#include "src/ui/camera/camera.h"
#include "engine/scene/scene.h"
#include "gui-framework/ui/lib.h"
#include "gui-framework/rhi/renderer.h"

/* === View === */

/* Proxy to bhs_camera_init */
void bhs_camera_init_view(bhs_camera_t *cam);

/* Proxy to bhs_camera_controller_update */
void bhs_camera_update_view(bhs_camera_t *cam, bhs_ui_ctx_t ctx, double dt);

struct bhs_planet_pass; /* Forward Declaration */

struct bhs_planet_tex_entry {
	char name[32];
	void *tex;
};

/* Asset container to avoid void* hacks */
typedef struct bhs_view_assets {
	void *bg_texture;
	void *sphere_texture;
	void *bh_texture; /* [NEW] Black Hole Compute Result */
	bool show_grid; /* Toggles wireframe rendering */
	const struct bhs_fabric *fabric; /* [NEW] Doppler Fabric Data */
	
	/* [NEW] Procedural Cache */
	const struct bhs_planet_tex_entry *tex_cache;
	int tex_cache_count;
} bhs_view_assets_t;

/* Proxy to renderer */
/* Proxy to renderer */
void bhs_view_spacetime_draw(bhs_ui_ctx_t ctx, bhs_scene_t scene,
			     const bhs_camera_t *cam, int width, int height,
			     const bhs_view_assets_t *assets,
			     struct bhs_planet_pass *planet_pass);

#endif /* BHS_UX_UI_VIEW_SPACETIME_H */
