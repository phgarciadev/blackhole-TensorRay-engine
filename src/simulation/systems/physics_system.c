/**
 * @file physics_system.c
 * @brief Sistema de Física Unificado (Adapter para Integrador High-Fidelity)
 */

#include "src/simulation/systems/systems.h"
#include "engine/physics/integrator.h"
#include "engine/components/components.h"
#include "src/simulation/components/sim_components.h"
#include <string.h>
#include <stdio.h>

void physics_system_update(bhs_world_handle world, double dt)
{
    /* 1. Extract ECS data to Integrator State */
    struct bhs_system_state state;
    state.n_bodies = 0;
    state.time = 0.0; // Cumulative time handled by app_state, this is local step

    bhs_ecs_query q;
    /* We need Transform (Pos) and Physics (Vel, Mass) */
    bhs_ecs_query_init(&q, world, (1 << BHS_COMP_PHYSICS) | (1 << BHS_COMP_TRANSFORM));
    
    /* Map IDs to array indices to write back later */
    bhs_entity_id entity_map[BHS_MAX_BODIES];
    
    bhs_entity_id id;
    while(bhs_ecs_query_next(&q, &id) && state.n_bodies < BHS_MAX_BODIES) {
        bhs_transform_t *t = bhs_ecs_get_component(world, id, BHS_COMP_TRANSFORM);
        bhs_physics_t *p = bhs_ecs_get_component(world, id, BHS_COMP_PHYSICS);
        
        if (!t || !p) continue;

        int i = state.n_bodies;
        state.bodies[i].pos = t->position;
        state.bodies[i].vel = p->velocity;
        state.bodies[i].mass = p->mass;
        state.bodies[i].gm = p->mass * 6.67430e-11; /* G_SI */
        state.bodies[i].is_fixed = p->is_static;
        state.bodies[i].is_alive = true;
        
        entity_map[i] = id;
        state.n_bodies++;
    }

    /* 2. Run High-Fidelity Integrator (Leapfrog) */
    /* This handles Gravity (N^2), 1PN Relativity, and Symplectic Integration */
    bhs_integrator_leapfrog(&state, dt);

    /* 3. Write back to ECS */
    for (int i = 0; i < state.n_bodies; i++) {
        bhs_entity_id eid = entity_map[i];
         // Only update if not static (though integrator handles fixed flag, 
         // we double check to avoid dirtying cache lines if needed)
        if (state.bodies[i].is_fixed) continue; 

        bhs_transform_t *t = bhs_ecs_get_component(world, eid, BHS_COMP_TRANSFORM);
        bhs_physics_t *p = bhs_ecs_get_component(world, eid, BHS_COMP_PHYSICS);
        
        if (t) t->position = state.bodies[i].pos;
        if (p) p->velocity = state.bodies[i].vel;
    }
}
