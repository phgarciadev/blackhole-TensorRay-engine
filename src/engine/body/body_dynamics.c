/**
 * @file body_dynamics.c
 * @brief Dinâmica e Integração de Corpos
 */

#include "body.h"

void bhs_body_integrate(struct bhs_body *b, double dt) {
  /*
   * Integração de Euler semi-implícita (Symplectic Euler)
   * Simples, robusta, conserva energia melhor que Euler explícito.
   * O chamador deve atualizar b->vel (com forças) ANTES de chamar isso.
   */
  if (!b)
    return;

  b->pos.x += b->vel.x * dt;
  b->pos.y += b->vel.y * dt;
  b->pos.z += b->vel.z * dt;
}
