#ifndef BHS_UX_UI_VIEW_SPACETIME_H
#define BHS_UX_UI_VIEW_SPACETIME_H

#include "src/ui/camera/camera.h"
#include "engine/scene/scene.h"
#include "framework/ui/lib.h"
#include "framework/rhi/renderer.h"

/* === View === */

/* Proxy to bhs_camera_init */
void bhs_camera_init_view(bhs_camera_t *cam);

/* Proxy to bhs_camera_controller_update */
void bhs_camera_update_view(bhs_camera_t *cam, bhs_ui_ctx_t ctx, double dt);

/* Asset container to avoid void* hacks */
typedef struct bhs_view_assets {
	void *bg_texture;
	void *sphere_texture;
	bool show_grid; /* Toggles wireframe rendering */
} bhs_view_assets_t;

/* Proxy to renderer */
void bhs_view_spacetime_draw(bhs_ui_ctx_t ctx, bhs_scene_t scene,
			     const bhs_camera_t *cam, int width, int height,
			     const bhs_view_assets_t *assets);

#endif /* BHS_UX_UI_VIEW_SPACETIME_H */
