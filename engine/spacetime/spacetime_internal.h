#ifndef BHS_ENGINE_SPACETIME_INTERNAL_H
#define BHS_ENGINE_SPACETIME_INTERNAL_H

#include "spacetime.h"

/**
 * @brief Estrutura interna da Malha Espaço-Tempo
 *
 * Definida aqui para ser compartilhada entre data.c e physics.c,
 * mas oculta do resto do sistema.
 */
struct bhs_spacetime_impl {
  double size;
  int divisions;
  int num_vertices;

  /*
   * Dados brutos para renderização:
   * x, y, z, r, g, b (6 floats por vértice)
   */
  float *vertex_data;
};

#endif /* BHS_ENGINE_SPACETIME_INTERNAL_H */
