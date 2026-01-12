/**
 * @file spacetime_renderer.c
 * @brief Renderização Pura da Malha (Projeção e Desenho de Linhas)
 */

#include "spacetime_renderer.h"
#include "engine/spacetime/spacetime.h"
#include <math.h>

/* Helper: Projeta world -> screen */
static void project_point(const bhs_camera_t *cam, float x, float y, float z,
                          float sw, float sh, float *ox, float *oy) {
  float px = x - cam->x;
  float py = y - cam->y; /* Y da camera é altura */
  float pz = z - cam->z;

  if (pz < 1.0f)
    pz = 1.0f;

  float factor = cam->fov / pz;

  float proj_x = px * factor;
  float proj_y = py * factor;

  *ox = proj_x + sw * 0.5f;
  /* Inverte Y da tela */
  *oy = (sh * 0.5f) - proj_y;
}

void bhs_spacetime_renderer_draw(bhs_ui_ctx_t ctx, bhs_scene_t scene,
                                 const bhs_camera_t *cam, int width,
                                 int height) {
  bhs_spacetime_t st = bhs_scene_get_spacetime(scene);
  if (!st)
    return;

  float *vertices;
  int count;
  bhs_spacetime_get_render_data(st, &vertices, &count);
  if (!vertices)
    return;

  int divs = bhs_spacetime_get_divisions(st);
  int cols = divs + 1;
  int rows = divs + 1;

  struct bhs_ui_color col_base = {0.0f, 0.8f, 1.0f, 0.3f};
  struct bhs_ui_color col_hilit = {0.5f, 0.9f, 1.0f, 0.8f};

  for (int r = 0; r < rows; r++) {
    for (int c = 0; c < cols; c++) {
      int idx = (r * cols + c) * 6;

      float x1 = vertices[idx + 0];
      float y1 = vertices[idx + 1]; /* Deformação (Y físico) */
      float z1 = vertices[idx + 2];

      float sx1, sy1;
      project_point(cam, x1, y1, z1, width, height, &sx1, &sy1);

      bool deep = fabs(y1) > 0.5f;
      struct bhs_ui_color c1 = deep ? col_hilit : col_base;
      float t1 = deep ? 2.0f : 1.0f;

      /* Horizontal (Right) */
      if (c < cols - 1) {
        int idx2 = (r * cols + c + 1) * 6;
        float x2 = vertices[idx2 + 0];
        float y2 = vertices[idx2 + 1];
        float z2 = vertices[idx2 + 2];

        float sx2, sy2;
        project_point(cam, x2, y2, z2, width, height, &sx2, &sy2);

        bhs_ui_draw_line(ctx, sx1, sy1, sx2, sy2, c1, t1);
      }

      /* Vertical (Down) */
      if (r < rows - 1) {
        int idx2 = ((r + 1) * cols + c) * 6;
        float x2 = vertices[idx2 + 0];
        float y2 = vertices[idx2 + 1];
        float z2 = vertices[idx2 + 2];

        float sx2, sy2;
        project_point(cam, x2, y2, z2, width, height, &sx2, &sy2);

        bhs_ui_draw_line(ctx, sx1, sy1, sx2, sy2, c1, t1);
      }
    }
  }
}
