/**
 * @file physics.c
 * @brief Implementação do Motor de Física (Compute Shader)
 *
 * "Se você não consegue explicar simplesmente, você não entende bem o
 * suficiente."
 * - Einstein (tentando debugar um shader)
 */

#include "physics.h"
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

  /* 1. Cria textura de saída (Storage Image) */
  struct bhs_gpu_texture_config tex_cfg = {
      .width = config->width,
      .height = config->height,
      .format = BHS_FORMAT_RGBA8_UNORM,
      .usage = BHS_TEXTURE_STORAGE | BHS_TEXTURE_SAMPLED,
      .label = "Physics Output",
  };

  /* Pass 1: Texture Output */
  if (bhs_gpu_texture_create(config->device, &tex_cfg, &p->output_texture) !=
      BHS_GPU_OK) {
    fprintf(stderr, "[physics] erro: falha ao criar textura de saída\n");
    free(p);
    return -1;
  }

  /* 2. Cria sampler */
  struct bhs_gpu_sampler_config sampler_cfg = {
      .min_filter = BHS_FILTER_LINEAR,
      .mag_filter = BHS_FILTER_LINEAR,
      .address_u = BHS_ADDRESS_CLAMP_TO_EDGE,
      .address_v = BHS_ADDRESS_CLAMP_TO_EDGE,
  };

  /* Pass 2: Sampler */
  if (bhs_gpu_sampler_create(config->device, &sampler_cfg, &p->sampler) !=
      BHS_GPU_OK) {
    fprintf(stderr, "[physics] erro: falha ao criar sampler\n");
    bhs_gpu_texture_destroy(p->output_texture);
    free(p);
    return -1;
  }

  /* 3. Carrega shader modular fiel */
  size_t shader_size;
  void *shader_code =
      read_file("build/engine/shaders/grid_fiel.comp.spv", &shader_size);

  if (!shader_code || shader_size == 0) {
    fprintf(stderr, "[physics] erro: falha ao carregar grid_fiel.comp.spv\n");
    bhs_gpu_sampler_destroy(p->sampler);
    bhs_gpu_texture_destroy(p->output_texture);
    free(p);
    return -1;
  }

  struct bhs_gpu_shader_config shader_cfg = {
      .stage = BHS_SHADER_COMPUTE,
      .code = shader_code,
      .code_size = shader_size,
      .entry_point = "main",
  };

  /* Pass 4: Creating Shader Module */
  if (bhs_gpu_shader_create(config->device, &shader_cfg, &p->shader) !=
      BHS_GPU_OK) {
    fprintf(stderr, "[physics] erro: falha ao criar shader\n");
    free(shader_code);
    bhs_gpu_sampler_destroy(p->sampler);
    bhs_gpu_texture_destroy(p->output_texture);
    free(p);
    return -1;
  }
  free(shader_code);

  /* 4. Cria compute pipeline */
  struct bhs_gpu_compute_pipeline_config pipe_cfg = {
      .compute_shader = p->shader,
  };

  /* Pass 5: Creating Compute Pipeline */
  if (bhs_gpu_pipeline_compute_create(config->device, &pipe_cfg,
                                      &p->pipeline) != BHS_GPU_OK) {
    fprintf(stderr, "[physics] erro: falha ao criar pipeline\n");
    bhs_gpu_shader_destroy(p->shader);
    bhs_gpu_sampler_destroy(p->sampler);
    bhs_gpu_texture_destroy(p->output_texture);
    free(p);
    return -1;
  }

  *physics = p;
  return 0;
}

void bhs_physics_destroy(bhs_physics_t physics) {
  if (!physics)
    return;

  bhs_gpu_pipeline_destroy(physics->pipeline);
  bhs_gpu_shader_destroy(physics->shader);
  bhs_gpu_sampler_destroy(physics->sampler);
  bhs_gpu_texture_destroy(physics->output_texture);
  free(physics);
}

void bhs_physics_step(bhs_physics_t physics, bhs_gpu_cmd_buffer_t cmd,
                      const struct bhs_physics_params *params) {
  if (!physics || !cmd || !params)
    return;

  /* Push constants para o shader modular */
  /* Alinhamento std430: vec2 exige 8 bytes. */
  struct {
    float time;
    float _pad;
    float resolution[2];
    float camera_pitch;
  } push = {
      .time = params->time,
      .resolution = {(float)physics->width, (float)physics->height},
      .camera_pitch = params->camera_incl,
  };

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
