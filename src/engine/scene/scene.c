/**
 * @file scene.c
 * @brief Implementação do Orquestrador
 *
 * "Gerenciando o caos, um frame de cada vez."
 */

#include "engine/scene/scene.h"
#include <stdio.h>
#include <stdlib.h>

#define MAX_BODIES 128

struct bhs_scene_impl {
  struct bhs_body bodies[MAX_BODIES];
  int n_bodies;

  bhs_spacetime_t spacetime;
};

bhs_scene_t bhs_scene_create(void) {
  bhs_scene_t scene = calloc(1, sizeof(struct bhs_scene_impl));
  if (!scene)
    return NULL;

  scene->n_bodies = 0;
  scene->spacetime = NULL;

  return scene;
}

void bhs_scene_destroy(bhs_scene_t scene) {
  if (!scene)
    return;

  if (scene->spacetime) {
    bhs_spacetime_destroy(scene->spacetime);
  }

  free(scene);
}

void bhs_scene_init_default(bhs_scene_t scene) {
  if (!scene)
    return;

  /* Limpa estado anterior se houver */
  scene->n_bodies = 0;
  if (scene->spacetime) {
    bhs_spacetime_destroy(scene->spacetime);
    scene->spacetime = NULL;
  }

  /* 1. Cria Malha do Espaço-Tempo (50x50 unidades, 100 divisões) */
  scene->spacetime = bhs_spacetime_create(50.0, 100);

  /* 2. Cria Gargantua (Buraco Negro Central) */
  /* Massa 5.0, Spin 0.5 (moderado) */
  if (scene->n_bodies < MAX_BODIES) {
    scene->bodies[scene->n_bodies++] =
        bhs_body_create_blackhole((struct bhs_vec3){0, 0, 0}, 5.0, 0.5);
  }

  /* 3. Cria Planeta Miller (Orbitando perigosamente perto) */
  /* Usando velocidade inicial tosca por enquanto (v = sqrt(GM/r) aponta pra z?)
   */
  if (scene->n_bodies < MAX_BODIES) {
    struct bhs_body miller =
        bhs_body_create_planet((struct bhs_vec3){15.0, 0, 0}, /* x=15 */
                               0.01, /* Massa pequena */
                               0.5,  /* Raio */
                               (struct bhs_vec3){0.2, 0.4, 1.0} /* Azulado */
        );
    /* Vel orbital circular approx: v = sqrt(M/r) = sqrt(5/15) = 0.577 */
    miller.vel = (struct bhs_vec3){0, 0, 0.577};

    scene->bodies[scene->n_bodies++] = miller;
  }

  /* 4. Cria Estrela distante (amarela) */
  if (scene->n_bodies < MAX_BODIES) {
    struct bhs_body star = bhs_body_create_planet(
        (struct bhs_vec3){-20.0, 0, -10.0}, 1.0, /* Massa considerável */
        1.5,                                     /* Grande */
        (struct bhs_vec3){1.0, 0.9, 0.2}         /* Amarelo */
    );
    star.type = BHS_BODY_STAR;
    scene->bodies[scene->n_bodies++] = star;
  }
}

void bhs_scene_update(bhs_scene_t scene, double dt) {
  if (!scene)
    return;

  /* 1. Integração de movimento dos corpos */
  /*
   * TODO: Implementar gravidade N-Corpos real.
   * Por enquanto, apenas move linearmente ou orbita fixa em torno da origem
   * (Assumindo que corpo 0 é estático na origem)
   */
  const struct bhs_body *central = &scene->bodies[0]; /* Assumindo BH */

  for (int i = 1; i < scene->n_bodies; i++) {
    struct bhs_body *b = &scene->bodies[i];

    /* Força gravitacional simplificada F = -GM/r^2 * r_hat */
    double dx = b->pos.x - central->pos.x;
    double dy = b->pos.y - central->pos.y;
    double dz = b->pos.z - central->pos.z;

    double r2 = dx * dx + dy * dy + dz * dz;
    double r = sqrt(r2);
    double r3 = r2 * r;

    if (r > 1.0) {               /* Evita singularidade numérica muito perto */
      double GM = central->mass; /* G=1 */
      double ax = -GM * dx / r3;
      double ay = -GM * dy / r3;
      double az = -GM * dz / r3;

      b->vel.x += ax * dt;
      b->vel.y += ay * dt;
      b->vel.z += az * dt;
    }

    bhs_body_integrate(b, dt);
  }

  /* 2. Atualiza deformação da malha */
  if (scene->spacetime) {
    bhs_spacetime_update(scene->spacetime, scene->bodies, scene->n_bodies);
  }
}

bhs_spacetime_t bhs_scene_get_spacetime(bhs_scene_t scene) {
  return scene ? scene->spacetime : NULL;
}

const struct bhs_body *bhs_scene_get_bodies(bhs_scene_t scene, int *count) {
  if (!scene) {
    *count = 0;
    return NULL;
  }
  *count = scene->n_bodies;
  return scene->bodies;
}
