/**
 * @file orbital_integrator.c
 * @brief Sistema de Integração de Movimento (Symplectic Euler)
 */

#include "engine/ecs/ecs.h"
#include "engine/components/components.h"

void orbital_integrator_system_update(bhs_world_handle world, double dt)
{
    bhs_ecs_query q;
    bhs_ecs_query_init(&q, world, (1 << BHS_COMP_PHYSICS) | (1 << BHS_COMP_TRANSFORM));
    
    bhs_entity_id id;
    while(bhs_ecs_query_next(&q, &id)) {
        bhs_transform_t *t = bhs_ecs_get_component(world, id, BHS_COMP_TRANSFORM);
        bhs_physics_t *p = bhs_ecs_get_component(world, id, BHS_COMP_PHYSICS);
        
        if (p->is_static) continue;

        /* Velocity already updated by gravity (Symplectic: Update V then Pos) */
        
        /* Update Position */
        t->position.x += p->velocity.x * dt;
        t->position.y += p->velocity.y * dt;
        t->position.z += p->velocity.z * dt;
        
        /* Update Rotation (Simple Spin) */
        /* TODO: Add angular velocity to physics component? 
           Using a fixed rotation for now if needed or reading from celestial component? 
           Let's keep it simple: planets spin. */
        
        // Simulating spin based on some arbitrary axis if not in component
        // Ideally we should have p->angular_velocity
    }
}
