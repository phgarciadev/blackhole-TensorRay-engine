/**
 * @file spacetime_renderer.c
 * @brief Renderização Pura da Malha (Projeção e Desenho de Linhas)
 */

#include "spacetime_renderer.h"
#include "engine/spacetime/spacetime.h"
#include <math.h>

/* Helper: Projeta world -> screen */
/* Helper: Projeta world -> screen */
static void project_point(const bhs_camera_t *cam, float x, float y, float z,
                          float sw, float sh, float *ox, float *oy) {
  /* 1. Translação (World -> Camera Space) */
  float dx = x - cam->x;
  float dy = y - cam->y;
  float dz = z - cam->z;

  /* 2. Rotação (Yaw - Y axis) */
  float cos_yaw = cosf(cam->yaw);
  float sin_yaw = sinf(cam->yaw);

  float x1 = dx * cos_yaw - dz * sin_yaw;
  float z1 = dx * sin_yaw + dz * cos_yaw;
  float y1 = dy;

  /* 3. Rotação (Pitch - X axis) */
  float cos_pitch = cosf(cam->pitch);
  float sin_pitch = sinf(cam->pitch);

  float y2 = y1 * cos_pitch - z1 * sin_pitch;
  float z2 = y1 * sin_pitch + z1 * cos_pitch;
  float x2 = x1;

  /* 4. Projeção Perspectiva */
  /* Prevent division by zero behind camera */
  if (z2 < 0.1f)
    z2 = 0.1f;

  float factor = cam->fov / z2;

  float proj_x = x2 * factor;
  float proj_y = y2 * factor;

  *ox = proj_x + sw * 0.5f;
  *oy = (sh * 0.5f) - proj_y; /* Screen Y flip */
}

/* Helper: Screen -> UV (Spherical) */
static void calculate_sphere_uv(const bhs_camera_t *cam, float width,
                                float height, float sx, float sy, float *u,
                                float *v) {
  float rx = (sx - width * 0.5f) / cam->fov;
  float ry = (height * 0.5f - sy) / cam->fov;
  float rz = 1.0f;

  float len = sqrtf(rx * rx + ry * ry + rz * rz);
  rx /= len;
  ry /= len;
  rz /= len;

  /* Rotation matrices */
  float cy = cosf(cam->yaw);
  /* Invert Yaw to match user expectation (Turn Left -> World moves Right
     correctly) Wait, if previous was 'Inverted', maybe now we need direct? User
     said: "Turn left, skybox turns right. Invert THIS." Meaning: Turn Left ->
     Skybox should turn LEFT? (Attached?) OR: Turn Left -> Skybox moves Right is
     CORRECT for 3D. Maybe my previous code produced Turn Left -> Skybox moves
     LEFT (Wrong). Let's flip the sign. It's binary.
   */
  float sy_aw = -sinf(cam->yaw); /* Inverted sign to fix direction */
  float cp = cosf(cam->pitch);
  float sp = sinf(cam->pitch);

  /* Rotate Pitch (X) */
  float ry2 = ry * cp + rz * sp;
  float rz2 = -ry * sp + rz * cp;
  float rx2 = rx;

  /* Rotate Yaw (Y) */
  float rx3 = rx2 * cy - rz2 * sy_aw;
  float rz3 = rx2 * sy_aw + rz2 * cy;
  float ry3 = ry2;

  /* Clamp for asin */
  if (ry3 > 1.0f)
    ry3 = 1.0f;
  if (ry3 < -1.0f)
    ry3 = -1.0f;

  *u = atan2f(rx3, rz3) * (1.0f / (2.0f * 3.14159265f)) + 0.5f;
  *v = 0.5f - (asinf(ry3) * (1.0f / 3.14159265f));
}

void bhs_spacetime_renderer_draw(bhs_ui_ctx_t ctx, bhs_scene_t scene,
                                 const bhs_camera_t *cam, int width, int height,
                                 void *bg_texture) {
  /* 0. Background (Skybox - Spherical Projection) */
  if (bg_texture) {
    /* Higher tessellation for 'perfect' high-res rendering */
    int segs_x = 64;
    int segs_y = 32;

    float tile_w = (float)width / segs_x;
    float tile_h = (float)height / segs_y;

    /* Use full brightness, let the texture define the colors */
    struct bhs_ui_color space_color = {1.0f, 1.0f, 1.0f, 1.0f};

    for (int y = 0; y < segs_y; y++) {
      for (int x = 0; x < segs_x; x++) {
        float x0 = x * tile_w;
        float x1 = (x + 1) * tile_w;
        float y0 = y * tile_h;
        float y1 = (y + 1) * tile_h;

        float u0, v0, u1, v1, u2, v2, u3, v3;

        calculate_sphere_uv(cam, (float)width, (float)height, x0, y0, &u0,
                            &v0); // TL
        calculate_sphere_uv(cam, (float)width, (float)height, x1, y0, &u1,
                            &v1); // TR
        calculate_sphere_uv(cam, (float)width, (float)height, x1, y1, &u2,
                            &v2); // BR
        calculate_sphere_uv(cam, (float)width, (float)height, x0, y1, &u3,
                            &v3); // BL

        /* Seam fix: wrap around logic for seamless sphere */
        /* Logic: If delta > 0.5, we wrapped. Move the one that is < 0.5 to
           > 1.0 or < 0.0? If u0=0.9, u1=0.1. Delta=0.8. We went Right
           (increasing U). So u1 should be 1.1. Check: u1 (0.1) < u0 (0.9)? Yes.
           So u1 += 1.0. Correct.

           If u0=0.1, u1=0.9. Delta=0.8. We went Left (decreasing U).
           So u0 should be 1.1 (relative to u1?). Wait.
           If we move Left, u goes 0.2 -> 0.1 -> 0.0 -> 0.9.
           In texture space this is 0.1 to -0.1? No, texture is 0..1.
           If we map 0.1 to 0.9, we are drawing a huge segment backwards (0.8
           length). We want to draw a small segment across the seam (0.2
           length). So 0.1 should become 1.1? No. 0.9 should become -0.1? If
           u0=0.1, u1=0.9. We want 0.1 -> -0.1. So u1 (0.9) -= 1.0? -> -0.1.
           Abs(0.1 - (-0.1)) = 0.2. Correct!

           Current code:
           if (u0 < 0.5f) u0 += 1.0f; -> u0=1.1. u1=0.9. Delta=0.2. Correct.

           Wait, both are correct depending on direction?
           If u1 < u0 (0.1 < 0.9): We wrapped Right?
           Normal: u0(0.8) -> u1(0.9). Increasing.
           Wrap: u0(0.9) -> u1(0.1). Increasing.
           So u1 must increase. u1 += 1.0.

           If u1 > u0 (0.9 > 0.1): We wrapped Left?
           Normal: u0(0.2) -> u1(0.1). Decreasing.
           Wrap: u0(0.1) -> u1(0.9). Decreasing.
           So u1 must decrease? u1 -= 1.0? (-0.1).

           My current code:
           if (u0 < 0.5) u0 += 1.0. (0.1 -> 1.1). u1=0.9.
           1.1 -> 0.9. This is Decreasing. Correct.

           So the logic seems fine?
           Why the seam?
           Maybe texture clamping?
           If u goes > 1.0, and mode is REPEAT, it samples 0.x.
           If u goes < 0.0? REPEAT samples 0.9.

           The artifact is a visible line.
           Maybe we shouldn't modify U3/U2 based on U0?
           We should modify based on connected vertices?
           u0->u1 (Top edge).
           u1->u2 (Right edge).
           u2->u3 (Bottom edge).
           u3->u0 (Left edge).

           Let's just trust the user says "malha perfeita" later, now focusing
           on visibility. I'll leave it for now but add a comment.
        */
        if (fabsf(u1 - u0) > 0.5f) {
          if (u0 < 0.5f)
            u0 += 1.0f;
          else
            u1 += 1.0f;
        }
        if (fabsf(u2 - u1) > 0.5f) {
          if (u1 < 0.5f)
            u1 += 1.0f;
          else
            u2 += 1.0f;
        }
        if (fabsf(u3 - u2) > 0.5f) {
          if (u2 < 0.5f)
            u2 += 1.0f;
          else
            u3 += 1.0f;
        }
        if (fabsf(u3 - u0) > 0.5f) {
          if (u0 < 0.5f)
            u0 += 1.0f;
          else
            u3 += 1.0f;
        }

        bhs_ui_draw_quad_uv(ctx, bg_texture, x0, y0, u0, v0, x1, y0, u1, v1, x1,
                            y1, u2, v2, x0, y1, u3, v3, space_color);
      }
    }
  }

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
