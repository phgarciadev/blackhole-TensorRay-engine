/**
 * @file physics_system.c
 * @brief Implementação da Física CPU
 */

#include "engine/systems/physics_system.h"
#include <stdio.h>
#include "engine/components/components.h"

void bhs_physics_system_update(bhs_world_handle world, real_t dt)
{
	// Na implementação ECS simples (Array of Structs per Component),
	// podemos iterar over todos os IDs válidos.
	// Otimização: Iterar apenas no menor array (Physics ou Transform) e checar o outro.

	// Por simplicidade (ID 1 to MAX):
	for (bhs_entity_id id = 1; id < BHS_MAX_ENTITIES; id++) {
		bhs_transform_component *transform =
			bhs_ecs_get_component(world, id, BHS_COMP_TRANSFORM);
		bhs_physics_component *physics =
			bhs_ecs_get_component(world, id, BHS_COMP_PHYSICS);

		if (transform && physics) {
			// Integração Semi-Implícita de Euler
			// 1. Update Velocidade: v = v + (F/m)*dt
			//    F = force_accumulator (gravity, input)
			//    (Mock Gravity aqui se necessário)

			struct bhs_vec4 accel =
				bhs_vec4_scale(physics->force_accumulator,
					       physics->inverse_mass);

			physics->velocity = bhs_vec4_add(
				physics->velocity, bhs_vec4_scale(accel, dt));

			// 2. Update Posição: p = p + v*dt
			transform->position = bhs_vec4_add(
				transform->position,
				bhs_vec4_scale(physics->velocity, dt));

			// 3. Reset forces
			physics->force_accumulator = bhs_vec4_zero();

			// 4. Drag (Opcional)
			physics->velocity = bhs_vec4_scale(
				physics->velocity, 1.0 - (physics->drag * dt));
		}
	}
}
