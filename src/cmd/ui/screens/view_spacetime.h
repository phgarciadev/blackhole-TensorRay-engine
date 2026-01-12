#ifndef BHS_UX_UI_VIEW_SPACETIME_H
#define BHS_UX_UI_VIEW_SPACETIME_H

#include "cmd/ui/camera/camera.h"
#include "engine/scene/scene.h"
#include "lib/ui_framework/lib.h"

/* === View === */

/* Proxy to bhs_camera_init */
void bhs_camera_init_view(bhs_camera_t *cam);

/* Proxy to bhs_camera_controller_update */
void bhs_camera_update_view(bhs_camera_t *cam, bhs_ui_ctx_t ctx, double dt);

/* Proxy to renderer */
void bhs_view_spacetime_draw(bhs_ui_ctx_t ctx, bhs_scene_t scene,
                             const bhs_camera_t *cam, int width, int height);

#endif /* BHS_UX_UI_VIEW_SPACETIME_H */
