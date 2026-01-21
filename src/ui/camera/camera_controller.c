/**
 * @file camera_controller.c
 * @brief Lógica de controle da câmera (WASD, Setas, Zoom)
 */

#include "camera_controller.h"
#include <math.h>
#include "gui/ui/lib.h"

void bhs_camera_controller_update(bhs_camera_t *cam, bhs_ui_ctx_t ctx,
				  double dt)
{
	if (!cam)
		return;

	/* Mouse Pan (Left Button + Drag) */
	{
		static int32_t last_mx = 0;
		static int32_t last_my = 0;
		static bool is_dragging = false;

		if (bhs_ui_mouse_down(ctx, 0)) { /* Left Button */
			int32_t mx, my;
			bhs_ui_mouse_pos(ctx, &mx, &my);

			if (!is_dragging) {
				/* First frame of click: Start tracking */
				is_dragging = true;
				last_mx = mx;
				last_my = my;
			} else {
				/* Update Position */
				float dx = (float)(mx - last_mx);
				float dy = (float)(my - last_my);

				/* Apply Rotation (Sensitivity) */
				/* dx -> Yaw (Virar esq/dir) */
				cam->yaw += dx * 0.005f;

				/* dy -> Pitch (Olhar cima/baixo) */
				/* Standard FPS: Mouse Down (dy > 0) -> Pitch Decrease (Look Down, assuming pitch is elevation?)
           Wait, if Pitch=0 is horizon. Pitch > 0 is Up?
           Let's standard: Pitch += -dy (Mouse Up -> Look Up) */
				cam->pitch -= dy * 0.005f;

				last_mx = mx;
				last_my = my;
			}
		} else {
			is_dragging = false;
		}
	}

	/* Keyboard Controls (Supplemental) */
	/* Keyboard Controls (FPS Style: Move relative to Look) */
	/* SI Scale: 1 AU = 1.5e11m. Speed needs to be ~0.1 AU/sec to be usable */
	/* Speed: 2.0e9 m/s (~6x Speed of Light) */
	float move_speed = 2.0e9f * (float)dt;
	
	/* Shift Boost */
	if (bhs_ui_key_down(ctx, BHS_KEY_LEFTSHIFT)) {
		move_speed *= 10.0f; /* 60c */
	}

	/* Calculate Forward and Right vectors */
	/* Forward: Z is forward (usually -Z in OpenGL, but here +Z seems to be 'into'
     based on previous logic?) Wait, init said z = -40, so looking +Z? Let's
     standard: Yaw 0 = Looking +Z? Let's deduce: x = sin(yaw) z = cos(yaw)
  */
	/* Calculate Forward and Right vectors */
	/* Forward: Z is forward (usually -Z in OpenGL, but here +Z seems to be 'into'
     based on previous logic?) Wait, init said z = -40, so looking +Z? Let's
     standard: Yaw 0 = Looking +Z? Let's deduce: x = sin(yaw) z = cos(yaw)
  */
	float sin_y = sinf(cam->yaw);
	float cos_y = cosf(cam->yaw);

	/* Mouse Zoom (3D Directional) */
	float scroll = bhs_ui_mouse_scroll(ctx);
	if (scroll != 0.0f) {
		float zoom_speed = 5.0e8f; /* Sensitivity */
		
        /* Calculate 3D Forward Vector */
        /* Pitch affects how much we move in Y vs XZ plane */
        /* If looking down (Pitch < 0), we move down. */
        float sin_p = sinf(cam->pitch);
        float cos_p = cosf(cam->pitch);

        /* Move along View Vector */
		cam->x += sin_y * cos_p * scroll * zoom_speed;
		cam->z += cos_y * cos_p * scroll * zoom_speed;
        cam->y += sin_p * scroll * zoom_speed; /* Check sign: Pitch-Up should move Up */
	}

	/* W/S - Forward/Back (Standard FPS: Forward moves INTO scene - Planar for walking) */
	if (bhs_ui_key_down(ctx, BHS_KEY_W)) {
		cam->x += sin_y * move_speed;
		cam->z += cos_y * move_speed;
	}
	if (bhs_ui_key_down(ctx, BHS_KEY_S)) {
		cam->x -= sin_y * move_speed;
		cam->z -= cos_y * move_speed;
	}

	/* A/D - Strafe Left/Right (User said WASD inverted? Maybe A/D too? Let's assume yes) */
	if (bhs_ui_key_down(ctx, BHS_KEY_A)) {
		/* Left -> Right */
		cam->x -= cos_y * move_speed;
		cam->z += sin_y * move_speed;
	}
	if (bhs_ui_key_down(ctx, BHS_KEY_D)) {
		/* Right -> Left */
		cam->x += cos_y * move_speed;
		cam->z -= sin_y * move_speed;
	}

	/* Q/E - Up/Down - Keeping standard unless requested */
	if (bhs_ui_key_down(ctx, BHS_KEY_Q))
		cam->y += move_speed;
	if (bhs_ui_key_down(ctx, BHS_KEY_E))
		cam->y -= move_speed;
}
