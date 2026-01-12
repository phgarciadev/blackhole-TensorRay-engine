/**
 * @file spacetime_data.c
 * @brief Gerenciamento de Memória e Estrutura da Malha
 */

#include "spacetime_internal.h"
#include <stdlib.h>
#include <string.h>

bhs_spacetime_t bhs_spacetime_create(double size, int divisions) {
  bhs_spacetime_t st = calloc(1, sizeof(struct bhs_spacetime_impl));
  if (!st)
    return NULL;

  st->size = size;
  st->divisions = divisions;

  /* Grid de linhas: (divisions + 1) linhas horizontais e verticais */
  int lines = divisions + 1;
  st->num_vertices = lines * lines; /* Pontos de intersecção */

  /* Cada ponto precisa de 6 floats */
  st->vertex_data = malloc(st->num_vertices * 6 * sizeof(float));
  if (!st->vertex_data) {
    free(st);
    return NULL;
  }

  /* Inicializa grid plano (z=0) */
  float step = size / divisions;
  float offset = size / 2.0f;

  int idx = 0;
  for (int i = 0; i <= divisions; i++) {
    for (int j = 0; j <= divisions; j++) {
      float x = j * step - offset;
      float z = i * step - offset; // No nosso mundo 3D, Z é profundidade

      /* x, y, z */
      st->vertex_data[idx++] = x;
      st->vertex_data[idx++] = 0.0f; /* Y = 0 (plano) */
      st->vertex_data[idx++] = z;

      /* r, g, b (cyan estilo Tron) */
      st->vertex_data[idx++] = 0.0f;
      st->vertex_data[idx++] = 0.8f;
      st->vertex_data[idx++] = 1.0f;
    }
  }

  return st;
}

void bhs_spacetime_destroy(bhs_spacetime_t st) {
  if (!st)
    return;
  if (st->vertex_data)
    free(st->vertex_data);
  free(st);
}

void bhs_spacetime_get_render_data(bhs_spacetime_t st, float **vertices,
                                   int *count) {
  if (!st) {
    *vertices = NULL;
    *count = 0;
    return;
  }
  *vertices = st->vertex_data;
  *count = st->num_vertices;
}

int bhs_spacetime_get_divisions(bhs_spacetime_t st) {
  return st ? st->divisions : 0;
}
