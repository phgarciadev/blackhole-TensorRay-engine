#ifndef BHS_ENGINE_SCENE_NEW_H
#define BHS_ENGINE_SCENE_NEW_H

/* Typedef must be visible */
typedef struct bhs_scene_impl *bhs_scene_t;

#include "engine/components/body/body.h"
#include "engine/engine.h"
#include "engine/physics/spacetime/spacetime.h"

/* API */
bhs_scene_t bhs_scene_create(void);
void bhs_scene_destroy(bhs_scene_t scene);
void bhs_scene_init_default(bhs_scene_t scene);
void bhs_scene_update(bhs_scene_t scene, double dt);
bhs_spacetime_t bhs_scene_get_spacetime(bhs_scene_t scene);
const struct bhs_body *bhs_scene_get_bodies(bhs_scene_t scene, int *count);
bool bhs_scene_add_body_struct(bhs_scene_t scene, struct bhs_body b);
bool bhs_scene_add_body(bhs_scene_t scene, enum bhs_body_type type,
			struct bhs_vec3 pos, struct bhs_vec3 vel, double mass,
			double radius, struct bhs_vec3 color);
void bhs_scene_remove_body(bhs_scene_t scene, int index);

#endif
