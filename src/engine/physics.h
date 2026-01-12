/**
 * @file physics.h
 * @brief Interface do Motor de Física (Compute Shader)
 *
 * "Física é a poesia da natureza, escrita em linguagem matemática."
 * - Galileu (provavelmente não tinha acesso a shaders, coitado)
 */

#ifndef BHS_SIM_PHYSICS_H
#define BHS_SIM_PHYSICS_H

#include "hal/gpu/renderer.h"

/* ============================================================================
 * TIPOS
 * ============================================================================
 */

struct bhs_physics_config {
  bhs_gpu_device_t device;
  uint32_t width;
  uint32_t height;
};

struct bhs_physics_params {
  float time;
  float mass;
  float spin;         /* Parâmetro de spin a/M (0 = Schwarzschild, 1 = extremo) */
  float camera_dist;
  float camera_angle;
  float camera_incl;  /* Inclinação da câmera (0 = polo, π/2 = equador) */
  int render_mode;    /* 0 = Realista, 1 = Grid/Debug */
};

typedef struct bhs_physics_impl *bhs_physics_t;

/* ============================================================================
 * API
 * ============================================================================
 */

/**
 * Inicializa o motor de física (cria pipeline, textura de saída, etc.)
 */
int bhs_physics_create(const struct bhs_physics_config *config,
                       bhs_physics_t *physics);

/**
 * Destroi o motor de física
 */
void bhs_physics_destroy(bhs_physics_t physics);

/**
 * Executa um passo da simulação
 * @param cmd Command buffer para submeter o dispatch
 * @param params Parâmetros da simulação
 */
void bhs_physics_step(bhs_physics_t physics, bhs_gpu_cmd_buffer_t cmd,
                      const struct bhs_physics_params *params);

/**
 * Retorna a textura de saída (renderizada pelo compute shader)
 */
bhs_gpu_texture_t bhs_physics_get_output_texture(bhs_physics_t physics);

#endif /* BHS_SIM_PHYSICS_H */
