/**
 * @file body_factory.c
 * @brief Fábrica de Corpos Celestes
 */

#include "body.h"
#include <string.h>

struct bhs_body bhs_body_create_planet(struct bhs_vec3 pos, double mass,
                                       double radius, struct bhs_vec3 color) {
  struct bhs_body b = {0};
  b.pos = pos;
  b.mass = mass;
  b.radius = radius;
  b.color = color;
  b.type = BHS_BODY_PLANET;
  /* Velocidade zero por padrão, altere manualmente se quiser que ande */
  return b;
}

struct bhs_body bhs_body_create_blackhole(struct bhs_vec3 pos, double mass,
                                          double spin) {
  struct bhs_body b = {0};
  b.pos = pos;
  b.mass = mass;
  b.spin = spin;
  /* Buracos negros são... pretos? Ou invisíveis? */
  b.color = (struct bhs_vec3){0.0, 0.0, 0.0};
  /* Raio visual aprox. do horizonte para debug */
  b.radius = 2.0 * mass;
  b.type = BHS_BODY_BLACKHOLE;
  return b;
}
