/**
 * @file spacetime_physics.c
 * @brief Lógica Física da Malha (Gravidade e Deformação)
 */

#include "engine/body/body.h"
#include "spacetime_internal.h"
#include <math.h>

void bhs_spacetime_update(bhs_spacetime_t st, const void *bodies_ptr,
                          int n_bodies) {
  if (!st || !bodies_ptr)
    return;

  const struct bhs_body *bodies = (const struct bhs_body *)bodies_ptr;

  /* Itera sobre todos os vértices e aplica deformação */
  int stride = 6;
  for (int i = 0; i < st->num_vertices; i++) {
    float *v = &st->vertex_data[i * stride];

    float x = v[0];
    float z = v[2]; /* Z é a profundidade no grid original */

    /* Calcula potencial gravitacional total neste ponto */
    /* V = -Sum(M / r) */
    double potential = 0.0;

    for (int b = 0; b < n_bodies; b++) {
      double M = bodies[b].mass;
      if (M <= 0)
        continue;

      double dx = x - bodies[b].pos.x;
      double dz = z - bodies[b].pos.z;
      /* Ignora Y do corpo por enquanto, assume projeção no plano */

      double dist_sq = dx * dx + dz * dz;
      double dist = sqrt(dist_sq + 0.1); /* +0.1 para evitar singularidade */

      potential += M / dist;
    }

    /* Deforma Y baseado no potencial (profundidade visual) */
    /* Fator de escala visual arbitrário */
    v[1] = -potential * 2.0f;

    /* Color shift baseado na profundidade? Legal mas deixa pra depois */
  }
}
