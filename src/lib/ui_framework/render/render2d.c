/**
 * @file render2d.c
 * @brief Onde a mágica 2D acontece (ou deveria)
 *
 * Gerencia o loop de renderização, Render Pass e (futuramente) o pipeline.
 * Se você quer desenhar um quadrado, você pede com educação aqui.
 */

#include "lib/ui_framework/internal.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * ESTRUTURAS
 * ============================================================================
 */

struct bhs_ui_vertex {
  float position[2];
  float tex_coord[2];
  float color[4];
};

struct bhs_render_batch {
  bhs_gpu_texture_t texture;
  uint32_t offset;
  uint32_t count;
};

#define BHS_MAX_VERTICES 262144
#define BHS_MAX_INDICES (BHS_MAX_VERTICES * 6)

/* ============================================================================
 * HELPERS
 * ============================================================================
 */

static void *read_file(const char *filename, size_t *size) {
  FILE *f = fopen(filename, "rb");
  if (!f) {
    fprintf(stderr, "Erro: Arquivo '%s' não encontrado. Quem comeu?\n",
            filename);
    return NULL;
  }

  if (fseek(f, 0, SEEK_END) != 0) {
    fprintf(stderr, "Erro: fseek falhou em '%s'. O HD pifou?\n", filename);
    fclose(f);
    return NULL;
  }

  long tell = ftell(f);
  if (tell < 0) {
    fprintf(stderr, "Erro: ftell retornou negativo. Fisica quebrou.\n");
    fclose(f);
    return NULL;
  }
  *size = (size_t)tell;

  rewind(f);

  void *buffer = malloc(*size);
  if (!buffer) {
    fprintf(stderr, "Erro: Malloc falhou para %zu bytes. Compre mais RAM.\n",
            *size);
    fclose(f);
    return NULL;
  }

  if (fread(buffer, 1, *size, f) != *size) {
    fprintf(stderr, "Erro: fread incompleto. Arquivo mudou de tamanho?\n");
    free(buffer);
    fclose(f);
    return NULL;
  }

  fclose(f);
  return buffer;
}

/* ============================================================================
 * IMPLEMENTAÇÃO
 * ============================================================================
 */

int bhs_ui_render_init_internal(bhs_ui_ctx_t ctx) {
  if (!ctx)
    return BHS_UI_ERR_INVALID;

  /* 1. Cria Buffers (Host Visible) */
  struct bhs_gpu_buffer_config v_cfg = {
      .size = BHS_MAX_VERTICES * sizeof(struct bhs_ui_vertex),
      .usage = BHS_BUFFER_VERTEX,
      .memory = BHS_MEMORY_CPU_VISIBLE,
      .label = "UI Vertex Buffer",
  };
  if (bhs_gpu_buffer_create(ctx->device, &v_cfg, &ctx->vertex_buffer) !=
      BHS_GPU_OK) {
    return BHS_UI_ERR_GPU;
  }

  struct bhs_gpu_buffer_config i_cfg = {
      .size = BHS_MAX_INDICES * sizeof(uint32_t),
      .usage = BHS_BUFFER_INDEX,
      .memory = BHS_MEMORY_CPU_VISIBLE,
      .label = "UI Index Buffer",
  };
  if (bhs_gpu_buffer_create(ctx->device, &i_cfg, &ctx->index_buffer) !=
      BHS_GPU_OK) {
    return BHS_UI_ERR_GPU;
  }

  /* Map buffers */
  ctx->mapped_vertices = bhs_gpu_buffer_map(ctx->vertex_buffer);
  ctx->mapped_indices = bhs_gpu_buffer_map(ctx->index_buffer);

  if (!ctx->mapped_vertices || !ctx->mapped_indices) {
    return BHS_UI_ERR_GPU;
  }

  /* 2. Carrega Shaders */
  size_t vs_size, fs_size;
  void *vs_code = read_file("shaders/ui.vert.spv", &vs_size);
  void *fs_code = read_file("shaders/ui.frag.spv", &fs_size);

  if (!vs_code || !fs_code) {
    fprintf(stderr, "erro: falha ao carregar shaders UI\n");
    if (vs_code)
      free(vs_code);
    if (fs_code)
      free(fs_code);
    return BHS_UI_ERR_INIT;
  }

  struct bhs_gpu_shader_config vs_cfg = {
      .stage = BHS_SHADER_VERTEX,
      .code = vs_code,
      .code_size = vs_size,
      .entry_point = "main",
  };
  bhs_gpu_shader_t vs;
  if (bhs_gpu_shader_create(ctx->device, &vs_cfg, &vs) != BHS_GPU_OK) {
    return BHS_UI_ERR_GPU;
  }

  struct bhs_gpu_shader_config fs_cfg = {
      .stage = BHS_SHADER_FRAGMENT,
      .code = fs_code,
      .code_size = fs_size,
      .entry_point = "main",
  };
  bhs_gpu_shader_t fs;
  if (bhs_gpu_shader_create(ctx->device, &fs_cfg, &fs) != BHS_GPU_OK) {
    return BHS_UI_ERR_GPU;
  }

  free(vs_code);
  free(fs_code);

  /* 3. Pipeline */
  struct bhs_gpu_vertex_attr attrs[] = {
      {0, 0, BHS_FORMAT_RG32_FLOAT, offsetof(struct bhs_ui_vertex, position)},
      {1, 0, BHS_FORMAT_RG32_FLOAT, offsetof(struct bhs_ui_vertex, tex_coord)},
      {2, 0, BHS_FORMAT_RGBA32_FLOAT, offsetof(struct bhs_ui_vertex, color)},
  };
  struct bhs_gpu_vertex_binding binding = {0, sizeof(struct bhs_ui_vertex),
                                           false};

  struct bhs_gpu_blend_state blend = {
      .enabled = true,
      .src_color = BHS_BLEND_SRC_ALPHA,
      .dst_color = BHS_BLEND_ONE_MINUS_SRC_ALPHA,
      .color_op = BHS_BLEND_OP_ADD,
      .src_alpha = BHS_BLEND_ONE,
      .dst_alpha = BHS_BLEND_ZERO,
      .alpha_op = BHS_BLEND_OP_ADD,
  };

  enum bhs_gpu_texture_format color_fmt =
      BHS_FORMAT_BGRA8_UNORM; /* Swapchain default */

  struct bhs_gpu_pipeline_config pipe_cfg = {
      .vertex_shader = vs,
      .fragment_shader = fs,
      .vertex_attrs = attrs,
      .vertex_attr_count = 3,
      .vertex_bindings = &binding,
      .vertex_binding_count = 1,
      .primitive = BHS_PRIMITIVE_TRIANGLES,
      .cull_mode = BHS_CULL_NONE,
      .front_ccw = false,
      .depth_test = false,
      .depth_write = false,
      .blend_states = &blend,
      .blend_state_count = 1,
      .color_formats = &color_fmt,
      .color_format_count = 1,
      .depth_stencil_format = BHS_FORMAT_UNDEFINED,
      .label = "UI Pipeline 2D",
  };

  if (bhs_gpu_pipeline_create(ctx->device, &pipe_cfg, &ctx->pipeline_2d) !=
      BHS_GPU_OK) {
    return BHS_UI_ERR_GPU;
  }

  bhs_gpu_shader_destroy(vs);
  bhs_gpu_shader_destroy(fs);

  /* 4. White Texture 1x1 */
  struct bhs_gpu_texture_config tex_cfg = {
      .width = 1,
      .height = 1,
      .depth = 1,
      .format = BHS_FORMAT_RGBA8_UNORM,
      .usage = BHS_TEXTURE_SAMPLED | BHS_TEXTURE_TRANSFER_DST,
      .mip_levels = 1,
      .array_layers = 1,
      .label = "White Tex",
  };
  bhs_gpu_texture_create(ctx->device, &tex_cfg, &ctx->white_texture);
  uint32_t white_pixel = 0xFFFFFFFF;
  bhs_gpu_texture_upload(ctx->white_texture, 0, 0, &white_pixel, 4);

  /* 5. Sampler */
  struct bhs_gpu_sampler_config sam_cfg = {
      .min_filter = BHS_FILTER_LINEAR,
      .mag_filter = BHS_FILTER_LINEAR,
      .address_u = BHS_ADDRESS_REPEAT,
      .address_v = BHS_ADDRESS_REPEAT,
      .address_w = BHS_ADDRESS_REPEAT,
  };
  bhs_gpu_sampler_create(ctx->device, &sam_cfg, &ctx->default_sampler);

  return BHS_UI_OK;
}

void bhs_ui_render_shutdown_internal(bhs_ui_ctx_t ctx) {
  if (!ctx)
    return;

  if (ctx->pipeline_2d)
    bhs_gpu_pipeline_destroy(ctx->pipeline_2d);
  if (ctx->white_texture)
    bhs_gpu_texture_destroy(ctx->white_texture);
  if (ctx->default_sampler)
    bhs_gpu_sampler_destroy(ctx->default_sampler);

  bhs_gpu_buffer_unmap(ctx->vertex_buffer);
  bhs_gpu_buffer_unmap(ctx->index_buffer);
  bhs_gpu_buffer_destroy(ctx->vertex_buffer);
  bhs_gpu_buffer_destroy(ctx->index_buffer);
}

void bhs_ui_render_begin(bhs_ui_ctx_t ctx) {
  if (!ctx || !ctx->cmd)
    return;

  ctx->vertex_count = 0;
  ctx->index_count = 0;
  ctx->current_batch.count = 0;
  ctx->current_batch.offset = 0;
  ctx->current_batch.texture = ctx->white_texture;

  /* Setup Render Pass */
  bhs_gpu_texture_t tex = ctx->current_texture;
  if (!tex)
    return;

  struct bhs_gpu_color_attachment color_att = {
      .texture = tex,
      .load_action = BHS_LOAD_CLEAR,
      .store_action = BHS_STORE_STORE,
      .clear_color = {0.1f, 0.1f, 0.1f, 1.0f},
  };

  struct bhs_gpu_render_pass pass = {
      .color_attachments = &color_att,
      .color_attachment_count = 1,
  };

  /* bhs_gpu_cmd_reset(ctx->cmd); REMOVED: Managed externally via
   * bhs_ui_cmd_begin */
  /* bhs_gpu_cmd_begin(ctx->cmd); REMOVED */

  /* Transition Layout? The simple renderer abstraction does it?
     Usually helper functions in abstraction handle layout transitions inside
     render pass or barrier. Our implementation of `cmd_begin_render_pass` in
     vulkan handles standard transitions.
  */

  bhs_gpu_cmd_begin_render_pass(ctx->cmd, &pass);

  bhs_gpu_cmd_set_viewport(ctx->cmd, 0, 0, (float)ctx->width,
                           (float)ctx->height, 0.0f, 1.0f);
  bhs_gpu_cmd_set_scissor(ctx->cmd, 0, 0, ctx->width, ctx->height);
  bhs_gpu_cmd_set_pipeline(ctx->cmd, ctx->pipeline_2d);

  /* Push Constants (Scale/Translate) */
  float push[4];
  push[0] = 2.0f / ctx->width;
  push[1] = 2.0f / ctx->height;
  push[2] = -1.0f;
  push[3] = -1.0f;
  bhs_gpu_cmd_push_constants(ctx->cmd, 0, push, sizeof(push));

  bhs_gpu_cmd_set_vertex_buffer(ctx->cmd, 0, ctx->vertex_buffer, 0);
  bhs_gpu_cmd_set_index_buffer(ctx->cmd, ctx->index_buffer, 0, true);
}

static void flush_batch(bhs_ui_ctx_t ctx) {
  if (ctx->current_batch.count == 0)
    return;

  bhs_gpu_cmd_bind_texture(ctx->cmd, 0, 0, ctx->current_batch.texture,
                           ctx->default_sampler);
  bhs_gpu_cmd_draw_indexed(ctx->cmd, ctx->current_batch.count, 1,
                           ctx->current_batch.offset, 0, 0);

  ctx->current_batch.offset += ctx->current_batch.count;
  ctx->current_batch.count = 0;
}

void bhs_ui_render_end(bhs_ui_ctx_t ctx) {
  if (!ctx || !ctx->cmd)
    return;

  flush_batch(ctx);

  bhs_gpu_cmd_end_render_pass(ctx->cmd);
  /* Note: cmd_end, submit and present are handled by context.c:bhs_ui_end_frame
   */
}

void bhs_ui_draw_texture(bhs_ui_ctx_t ctx, void *texture_void, float x, float y,
                         float w, float h, struct bhs_ui_color color) {
  BHS_ASSERT(ctx != NULL);

  bhs_gpu_texture_t texture = (bhs_gpu_texture_t)texture_void;
  if (!texture)
    texture = ctx->white_texture;

  /* Check batch texture change */
  if (ctx->current_batch.texture != texture) {
    flush_batch(ctx);
    ctx->current_batch.texture = texture;
  }

  /* Check overflow */
  if (ctx->vertex_count + 4 > BHS_MAX_VERTICES)
    return; /* Overflow protection: ignora primitivas extras */

  struct bhs_ui_vertex *v =
      &((struct bhs_ui_vertex *)ctx->mapped_vertices)[ctx->vertex_count];
  uint32_t *i = &((uint32_t *)ctx->mapped_indices)[ctx->index_count];
  uint32_t idx = ctx->vertex_count;

  v[0] = (struct bhs_ui_vertex){
      {x, y}, {0, 0}, {color.r, color.g, color.b, color.a}};
  v[1] = (struct bhs_ui_vertex){
      {x + w, y}, {1, 0}, {color.r, color.g, color.b, color.a}};
  v[2] = (struct bhs_ui_vertex){
      {x + w, y + h}, {1, 1}, {color.r, color.g, color.b, color.a}};
  v[3] = (struct bhs_ui_vertex){
      {x, y + h}, {0, 1}, {color.r, color.g, color.b, color.a}};

  // esses cara otario q fica leno código....

  i[0] = idx + 0;
  i[1] = idx + 1;
  i[2] = idx + 2;
  i[3] = idx + 2;
  i[4] = idx + 3;
  i[5] = idx + 0;

  ctx->vertex_count += 4;
  ctx->index_count += 6;
  ctx->current_batch.count += 6;
}

#include "lib/ui_framework/render/font.h"

/* Compatibility helpers */
void bhs_ui_draw_rect(bhs_ui_ctx_t ctx, struct bhs_ui_rect rect,
                      struct bhs_ui_color color) {
  BHS_ASSERT(ctx != NULL);
  bhs_ui_draw_texture(ctx, NULL, rect.x, rect.y, rect.width, rect.height,
                      color);
}

void bhs_ui_draw_rect_outline(bhs_ui_ctx_t ctx, struct bhs_ui_rect rect,
                              struct bhs_ui_color color, float thickness) {
  /* Desenha 4 bordas */
  /* Top */
  bhs_ui_draw_rect(
      ctx, (struct bhs_ui_rect){rect.x, rect.y, rect.width, thickness}, color);
  /* Bottom */
  bhs_ui_draw_rect(ctx,
                   (struct bhs_ui_rect){rect.x,
                                        rect.y + rect.height - thickness,
                                        rect.width, thickness},
                   color);
  /* Left */
  bhs_ui_draw_rect(ctx,
                   (struct bhs_ui_rect){rect.x, rect.y + thickness, thickness,
                                        rect.height - 2.0f * thickness},
                   color);
  /* Right */
  bhs_ui_draw_rect(ctx,
                   (struct bhs_ui_rect){rect.x + rect.width - thickness,
                                        rect.y + thickness, thickness,
                                        rect.height - 2.0f * thickness},
                   color);
}

void bhs_ui_draw_line(bhs_ui_ctx_t ctx, float x1, float y1, float x2, float y2,
                      struct bhs_ui_color color, float thickness) {
  BHS_ASSERT(ctx != NULL);
  /* BHS_ASSERT(ctx != NULL); */
  /* if (!ctx) { */
  /*   printf("CTX NULL!\n"); */
  /*   return; */
  /* } */
  /* if (!ctx->mapped_vertices) { */
  /*   printf("MAPPED VERTICES NULL!\n"); */
  /*   return; */
  /* } */

  /* Verifica se cabe no buffer */
  if (ctx->vertex_count + 4 > BHS_MAX_VERTICES)
    return;

  /* Check batch texture (usa white texture) */
  if (ctx->current_batch.texture != ctx->white_texture) {
    flush_batch(ctx);
    ctx->current_batch.texture = ctx->white_texture; /* Pode ser NULL, mas ok */
  }

  /* Math: Vetor direção e normal */
  float dx = x2 - x1;
  float dy = y2 - y1;
  float len_sq = dx * dx + dy * dy;

  /* Evita divisão por zero para linhas degeneradas */
  if (len_sq < 0.0001f)
    return;

  float inv_len = 1.0f / sqrtf(len_sq);
  float nx = -dy * inv_len; /* Normal (-dy, dx) normalizada */
  float ny = dx * inv_len;

  /* Offset para espessura (metade pra cada lado) */
  float off_x = nx * (thickness * 0.5f);
  float off_y = ny * (thickness * 0.5f);

  struct bhs_ui_vertex *v =
      &((struct bhs_ui_vertex *)ctx->mapped_vertices)[ctx->vertex_count];
  uint32_t *i = &((uint32_t *)ctx->mapped_indices)[ctx->index_count];
  uint32_t idx = ctx->vertex_count;

  /* 4 vértices do quad que compõe a linha */
  /* V0: P1 + offset */
  v[0] = (struct bhs_ui_vertex){
      {x1 + off_x, y1 + off_y}, {0, 0}, {color.r, color.g, color.b, color.a}};
  /* V1: P1 - offset */
  v[1] = (struct bhs_ui_vertex){
      {x1 - off_x, y1 - off_y}, {0, 1}, {color.r, color.g, color.b, color.a}};
  /* V2: P2 - offset */
  v[2] = (struct bhs_ui_vertex){
      {x2 - off_x, y2 - off_y}, {1, 1}, {color.r, color.g, color.b, color.a}};
  /* V3: P2 + offset */
  v[3] = (struct bhs_ui_vertex){
      {x2 + off_x, y2 + off_y}, {1, 0}, {color.r, color.g, color.b, color.a}};

  /* Índices (dois triângulos) */
  i[0] = idx + 0;
  i[1] = idx + 1;
  i[2] = idx + 2;
  i[3] = idx + 2;
  i[4] = idx + 3;
  i[5] = idx + 0;

  ctx->vertex_count += 4;
  ctx->index_count += 6;
  ctx->current_batch.count += 6;
}

void bhs_ui_draw_text(bhs_ui_ctx_t ctx, const char *text, float x, float y,
                      float size, struct bhs_ui_color color) {
  BHS_ASSERT(ctx != NULL);
  if (!text)
    return;

  float start_x = x;
  float scale = size / 8.0f; /* Fonte base é 8x8 */

  while (*text) {
    char c = *text;

    if (c == '\n') {
      x = start_x;
      y += size; /* Line height = size */
      text++;
      continue;
    }

    if (c >= 0x20 && c <= 0x7E) {
      /* Desenha caractere pixel a pixel (Bitmap) */
      /* É ineficiente desenhar 64 quads por letra? Sim.
       * Funciona sem carregar textura de fonte? Sim.
       * É "Kernel Style" no sentido de "Faça funcionar simples"? Sim.
       */
      const uint8_t *glyph = bhs_font_8x8[c - 0x20];
      for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
          if (glyph[row] & (1 << (7 - col))) {
            bhs_ui_draw_rect(ctx,
                             (struct bhs_ui_rect){x + col * scale,
                                                  y + row * scale, scale,
                                                  scale},
                             color);
          }
        }
      }
    }

    x += size; /* Avança cursor (monospaced) */
    text++;
  }
}

void bhs_ui_clear(bhs_ui_ctx_t ctx, struct bhs_ui_color color) {
  BHS_ASSERT(ctx != NULL);

  /* Renderiza um Quad Fullscreen para limpeza manual do framebuffer.
   * Útil quando LoadOp=CLEAR não é suficiente ou quando se deseja limpar
   * apenas uma região específica pós-renderpass.
   */
  bhs_ui_draw_rect(
      ctx, (struct bhs_ui_rect){0, 0, (float)ctx->width, (float)ctx->height},
      color);
}
