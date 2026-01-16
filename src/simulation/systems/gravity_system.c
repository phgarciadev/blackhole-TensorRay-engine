/**
 * @file gravity_system.c
 * @brief Sistema de Gravidade Universal (N-Body)
 */

#include "engine/ecs/ecs.h"
#include "engine/components/components.h"
#include "src/simulation/components/sim_components.h" // For customized mass/tags
#include "engine/physics/spacetime/spacetime.h" // For relativistic corrections if needed
#include <math.h>

#define G_CONST 6.67430e-11

void gravity_system_update(bhs_world_handle world, double dt)
{
    /* 1. Query all bodies with Mass + Position */
    /* Note: In a real N-body simulation, we need O(N^2) or Barnes-Hut. 
       For < 100 bodies, O(N^2) is fine. */
    
    bhs_ecs_query q1;
    bhs_ecs_query_init(&q1, world, (1 << BHS_COMP_PHYSICS) | (1 << BHS_COMP_TRANSFORM));
    
    // We need to cache entities to iterate N^2
    // Limiting to 128 bodies for this simplified loop
    struct cached_body {
        bhs_entity_id id;
        struct bhs_vec3 pos;
        double mass;
    } bodies[128];
    int count = 0;
    
    bhs_entity_id id;
    while(bhs_ecs_query_next(&q1, &id) && count < 128) {
        bhs_transform_t *t = bhs_ecs_get_component(world, id, BHS_COMP_TRANSFORM);
        bhs_physics_t *p = bhs_ecs_get_component(world, id, BHS_COMP_PHYSICS);
        
        if (p->mass > 0) {
            bodies[count].id = id;
            bodies[count].pos = t->position;
            bodies[count].mass = p->mass;
            count++;
        }
    }
    
    /* 2. Compute Forces */
    for (int i = 0; i < count; i++) {
        struct bhs_vec3 total_acc = {0,0,0};
        
        for (int j = 0; j < count; j++) {
            if (i == j) continue;
            
            struct bhs_vec3 dir = {
                bodies[j].pos.x - bodies[i].pos.x,
                bodies[j].pos.y - bodies[i].pos.y,
                bodies[j].pos.z - bodies[i].pos.z
            };
            
            double dist_sq = dir.x*dir.x + dir.y*dir.y + dir.z*dir.z;
            double dist = sqrt(dist_sq);
            
            if (dist < 1.0) dist = 1.0; // Softening
            
            double f = (G_CONST * bodies[j].mass) / dist_sq;
            
            total_acc.x += f * (dir.x / dist);
            total_acc.y += f * (dir.y / dist);
            total_acc.z += f * (dir.z / dist);
        }
        
        /* 3. Apply to Velocity directly (Unit Mass) or Accumulate Force */
        bhs_physics_t *p = bhs_ecs_get_component(world, bodies[i].id, BHS_COMP_PHYSICS);
        if (!p->is_static) {
            p->velocity.x += total_acc.x * dt;
            p->velocity.y += total_acc.y * dt;
            p->velocity.z += total_acc.z * dt;
        }
    }
}
