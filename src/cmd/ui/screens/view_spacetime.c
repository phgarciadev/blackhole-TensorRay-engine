/**
 * @file view_spacetime.c
 * @brief Orquestrador da View (Cola: Scene + Camera + Renderer)
 */

#include "view_spacetime.h"
#include "cmd/ui/camera/camera_controller.h"
#include "cmd/ui/render/spacetime_renderer.h"

/* === Interface === */

void bhs_camera_init_view(bhs_camera_t *cam)
{
	bhs_camera_init(cam);
}

void bhs_camera_update_view(bhs_camera_t *cam, bhs_ui_ctx_t ctx, double dt)
{
	bhs_camera_controller_update(cam, ctx, dt);
}

void bhs_view_spacetime_draw(bhs_ui_ctx_t ctx, bhs_scene_t scene,
			     const bhs_camera_t *cam, int width, int height,
			     const bhs_view_assets_t *assets)
{
	/* Note: assets_void is treated as const bhs_view_assets_t* inside */
	bhs_spacetime_renderer_draw(ctx, scene, cam, width, height,
				    (const void *)assets);
}
