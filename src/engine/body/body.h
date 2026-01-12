/**
 * @file body.h
 * @brief Entidades celestes (Corpos)
 *
 * "Tudo no universo é um corpo. Menos sua opinião, que não importa."
 *
 * Define a estrutura básica de objetos físicos na simulação:
 * - Planetas
 * - Estrelas
 * - Buracos Negros
 *
 * Invariantes:
 * - Massa deve ser > 0 (matéria exótica não é suportada... ainda)
 * - Raio > 0 se detectável visualmente
 */

#ifndef BHS_ENGINE_BODY_H
#define BHS_ENGINE_BODY_H

#include "lib/core.h"
#include <stdbool.h>

/* ============================================================================
 * TIPOS
 * ============================================================================
 */

enum bhs_body_type {
  BHS_BODY_PLANET,    /* Planeta (massa desprezível para métrica) */
  BHS_BODY_STAR,      /* Estrela (fonte de luz) */
  BHS_BODY_BLACKHOLE, /* Buraco Negro (gerador de métrica) */
};

/**
 * struct bhs_body - Um corpo celeste
 */
struct bhs_body {
  struct bhs_vec3 pos; /* Posição no espaço (x, y, z) */
  struct bhs_vec3 vel; /* Velocidade (vx, vy, vz) */

  double mass;   /* Massa (M) */
  double radius; /* Raio visual (R) */

  /* Propriedades específicas */
  enum bhs_body_type type;

  /* Para Black Holes apenas */
  double spin; /* Momento angular a/M (0..1) */

  /* Visual */
  struct bhs_vec3 color; /* Cor RGB (emite ou reflete) */
};

/* ============================================================================
 * API
 * ============================================================================
 */

/**
 * bhs_body_create_planet - Cria um planeta
 * @mass: Massa relativa
 * @radius: Raio visual
 * @color: Cor do planeta
 */
struct bhs_body bhs_body_create_planet(struct bhs_vec3 pos, double mass,
                                       double radius, struct bhs_vec3 color);

/**
 * bhs_body_create_blackhole - Cria um buraco negro
 * @mass: Massa (define horizonte)
 * @spin: Spin (a/M)
 */
struct bhs_body bhs_body_create_blackhole(struct bhs_vec3 pos, double mass,
                                          double spin);

/**
 * bhs_body_integrate - Integra movimento do corpo (Newtoniano/Pós-Newtoniano)
 *
 * Simples Euler ou RK4 para corpos massivos que não são fótons.
 * Para fótons, veja geodesic.h.
 */
void bhs_body_integrate(struct bhs_body *b, double dt);

#endif /* BHS_ENGINE_BODY_H */
