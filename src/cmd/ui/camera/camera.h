#ifndef BHS_CMD_UI_CAMERA_CAMERA_H
#define BHS_CMD_UI_CAMERA_CAMERA_H

/**
 * @brief Estrutura da Câmera
 */
typedef struct bhs_camera {
  float x, y, z; /* Posição World (Y UP) */
  float fov;     /* Field of View / Scale Factor */
} bhs_camera_t;

/**
 * @brief Inicializa a câmera com valores padrão
 */
void bhs_camera_init(bhs_camera_t *cam);

#endif /* BHS_CMD_UI_CAMERA_CAMERA_H */
