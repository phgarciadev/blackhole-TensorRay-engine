/**
 * @file physics.c
 * @brief Implementação do Motor de Física (Compute Shader)
 *
 * "Se você não consegue explicar simplesmente, você não entende bem o
 * suficiente."
 * - Einstein (tentando debugar um shader)
 */

#include "physics.h"
#include "body/body.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * ESTRUTURA INTERNA
 * ============================================================================
 */

struct bhs_physics_impl {
  bhs_gpu_device_t device;
  bhs_gpu_pipeline_t pipeline;
  bhs_gpu_shader_t shader;
  bhs_gpu_texture_t output_texture;
  bhs_gpu_sampler_t sampler;
  uint32_t width;
  uint32_t height;

  /* Simulação */
  struct bhs_body planet;
};

/* ============================================================================
 * HELPER: Lê arquivo binário
 * ============================================================================
 */

static void *read_file(const char *path, size_t *size) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return NULL;

  fseek(f, 0, SEEK_END);
  *size = (size_t)ftell(f);
  fseek(f, 0, SEEK_SET);

  void *data = malloc(*size);
  if (data) {
    fread(data, 1, *size, f);
  }
  fclose(f);
  return data;
}

/* ============================================================================
 * IMPLEMENTAÇÃO
 * ============================================================================
 */

int bhs_physics_create(const struct bhs_physics_config *config,
                       bhs_physics_t *physics) {
  if (!config || !config->device || !physics)
    return -1;

  struct bhs_physics_impl *p = calloc(1, sizeof(*p));
  if (!p)
    return -1;

  p->device = config->device;
  p->width = config->width;
  p->height = config->height;

  /* Inicializa o planeta em órbita */
  /* Posição (10, 0, 0), massa pequena, raio visual 0.5, cor vermelha */
  /* Inicializa o planeta em órbita */
  /* Posição (10, 0, 0), massa pequena, raio visual 0.5, cor vermelha */
  p->planet = bhs_body_create_planet_simple(
      (struct bhs_vec3){12.0, 0.0, 0.0}, 0.01, 0.5,
      (struct bhs_vec3){1.0, 0.3, 0.3});

  /* Velocidade orbital approx v = sqrt(GM/r) */
  /* Assumindo M=4.0 (do shader) => v = sqrt(4/12) = 0.577 */
  p->planet.state.vel = (struct bhs_vec3){0.0, 0.58, 0.0};

  /* ... (skip texture creation) ... */
  /* ... (in step function) ... */

  /* Push constants para o shader modular */
  struct {
    float time;
    float _pad;
    float resolution[2];
    float camera_pitch;
    float _pad2[3];
    float planet_pos[3];
    float planet_radius;
  } push = {
      .time = params->time,
      .resolution = {(float)physics->width, (float)physics->height},
      .camera_pitch = params->camera_incl,
      ._pad2 = {0},
      .planet_pos = {(float)physics->planet.state.pos.x,
                     (float)physics->planet.state.pos.y,
                     (float)physics->planet.state.pos.z},
      .planet_radius = (float)physics->planet.state.radius,
  };

  /* Física no CPU: Gravidade Newtoniana simples (Central Mass M=4.0) */
  struct bhs_vec3 center = {0, 0, 0};
  struct bhs_vec3 diff = bhs_vec3_sub(center, physics->planet.state.pos);
  double r = bhs_vec3_norm(diff);
  if (r > 0.1) {
    double M = 4.0;             /* Massa do buraco negro/sol central */
    double force = M / (r * r); /* G=1 */
    struct bhs_vec3 dir = bhs_vec3_normalize(diff);
    struct bhs_vec3 acc = bhs_vec3_scale(dir, force);

    /* Symplectic Euler: v += a*dt, x += v*dt */
    double dt = 0.05;
    physics->planet.state.vel =
        bhs_vec3_add(physics->planet.state.vel, bhs_vec3_scale(acc, dt));
    bhs_body_integrate(&physics->planet, dt);
  }

  /* Bind pipeline e recursos */
  bhs_gpu_cmd_set_pipeline(cmd, physics->pipeline);
  bhs_gpu_cmd_push_constants(cmd, 0, &push, sizeof(push));

  /* Bind storage image (Output) */
  bhs_gpu_cmd_bind_compute_storage_texture(cmd, physics->pipeline, 0, 0,
                                           physics->output_texture);

  /* Dispatch */
  uint32_t groups_x = (physics->width + 15) / 16;
  uint32_t groups_y = (physics->height + 15) / 16;
  bhs_gpu_cmd_dispatch(cmd, groups_x, groups_y, 1);

  /* 2. Transição para leitura (SHADER_READ_ONLY) */
  bhs_gpu_cmd_transition_texture(cmd, physics->output_texture);
}

bhs_gpu_texture_t bhs_physics_get_output_texture(bhs_physics_t physics) {
  return physics ? physics->output_texture : NULL;
}
