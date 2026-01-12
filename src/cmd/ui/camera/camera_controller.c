/**
 * @file camera_controller.c
 * @brief Lógica de controle da câmera (WASD, Setas, Zoom)
 */

#include "camera_controller.h"
#include "lib/ui_framework/lib.h"

void bhs_camera_controller_update(bhs_camera_t *cam, bhs_ui_ctx_t ctx,
                                  double dt) {
  if (!cam)
    return;

  /* Ajuste de velocidade */
  float move_speed = 30.0f * (float)dt;

  /* Pan Vertical (Z) */
  if (bhs_ui_key_down(ctx, BHS_KEY_W) || bhs_ui_key_down(ctx, BHS_KEY_UP))
    cam->z += move_speed;
  if (bhs_ui_key_down(ctx, BHS_KEY_S) || bhs_ui_key_down(ctx, BHS_KEY_DOWN))
    cam->z -= move_speed;

  /* Pan Horizontal (X) */
  if (bhs_ui_key_down(ctx, BHS_KEY_A) || bhs_ui_key_down(ctx, BHS_KEY_LEFT))
    cam->x -= move_speed;
  if (bhs_ui_key_down(ctx, BHS_KEY_D) || bhs_ui_key_down(ctx, BHS_KEY_RIGHT))
    cam->x += move_speed;

  /* Zoom/Altura (Y) - Q/E */
  if (bhs_ui_key_down(ctx, BHS_KEY_Q))
    cam->y += move_speed;
  if (bhs_ui_key_down(ctx, BHS_KEY_E))
    cam->y -= move_speed;
}
