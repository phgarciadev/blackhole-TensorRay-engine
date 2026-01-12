/**
 * @file camera.c
 * @brief Dados e Inicialização da Câmera
 */

#include "camera.h"
#include <stddef.h>

void bhs_camera_init(bhs_camera_t *cam) {
  if (!cam)
    return;
  cam->x = 0.0f;
  cam->y = 15.0f;  /* Altura da camera (Y UP) */
  cam->z = -40.0f; /* Distancia */
  cam->fov = 500.0f;
}
