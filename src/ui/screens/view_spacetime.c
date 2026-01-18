/**
 * @file view_spacetime.c
 * @brief Orquestrador da View (Cola: Scene + Camera + Renderer)
 */

#include "src/simulation/data/planet.h"
#include "engine/assets/image_loader.h"
#include "src/ui/render/planet_renderer.h"
#include "view_spacetime.h"
#include "src/ui/camera/camera_controller.h"
#include "src/ui/render/spacetime_renderer.h"

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
			     const bhs_view_assets_t *assets,
			     struct bhs_planet_pass *planet_pass)
{
	if (!ctx) return;

	/* 1. Reset & Begin Frame in Renderer if needed? 
	   Renderer handles command buffer? Yes. 
	   We need to piggyback on the main command buffer.
	   Wait, UI lib obfuscates the command buffer. `bhs_ui_ctx_t`.
	   `bhs_planet_pass_draw` requires `bhs_gpu_cmd_buffer_t`.
	   We need to extract it from `ctx` or let `ctx` execute explicit callbacks.
	   Currently `bhs_gpu_cmd_buffer_t` is NOT exposed via `bhs_ui_ctx_t` publicly in simple headers?
	   Let's check `gui-framework/ui/lib.h`.
	
	   If not exposed, I have to add an accessor: `bhs_ui_get_current_cmd_buffer(ctx)`.
	*/
	
	/* Draw 2.5D Elements (Grid, Skybox, BH Quad) */
	/* Note: This function inside spacetime_renderer.c also iterates planets and draws 2D versions.
	   We should DISABLE planet drawing there.
	*/
	bhs_spacetime_renderer_draw(ctx, scene, cam, width, height, assets);

	/* Draw 3D Elements */
	if (planet_pass) {
		bhs_gpu_cmd_buffer_t cmd = (bhs_gpu_cmd_buffer_t)bhs_ui_get_current_cmd(ctx);
		if (cmd) {
			/* Start Render Pass? 
			   UI is likely already inside a RenderPass (the Swapchain one).
			   The 3D pipeline expects to run inside A render pass.
			   Since we share the framebuffer, we should inherit it.
			   How to deal with `bhs_gpu_cmd_begin_render_pass` inside `bhs_planet_pass_draw`?
			   I implemented `bhs_planet_pass_draw` WITHOUT `begin_render_pass`. It just draws.
			   So we are good IF we are inside one.
			   The UI `draw_frame` usually starts one.
			   But `bhs_ui_draw_...` queues commands? Or records immediately?
			   Immediate recording.
			*/
			bhs_planet_pass_draw(planet_pass, cmd, scene, cam, assets, (float)width, (float)height);
		}
	}
}
