#ifndef BHS_UX_UI_VIEW_SPACETIME_H
#define BHS_UX_UI_VIEW_SPACETIME_H

#include "src/ui/camera/camera.h"
#include "engine/scene/scene.h"
#include "gui/ui/lib.h"
#include "gui/rhi/rhi.h"

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
	void *bh_texture; /* Black Hole Compute Result */
	
	/* Procedural Cache */
	const struct bhs_planet_tex_entry *tex_cache;
	int tex_cache_count;
	
	/* 3D Renderer Status */
	bool render_3d_active;
} bhs_view_assets_t;

/* Proxy to renderer */
/* Proxy to renderer */
void bhs_view_spacetime_draw(bhs_ui_ctx_t ctx, bhs_scene_t scene,
			     const bhs_camera_t *cam, int width, int height,
			     const bhs_view_assets_t *assets,
			     struct bhs_planet_pass *planet_pass);

#endif /* BHS_UX_UI_VIEW_SPACETIME_H */
