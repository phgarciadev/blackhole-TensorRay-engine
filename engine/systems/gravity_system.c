/**
 * @file gravity_system.c
 * @brief Implementação do Sistema de Gravidade
 *
 * "Newton acertou pra velocidades baixas. Einstein acertou sempre.
 * Nós vamos implementar Newton porque é mais rápido. Desculpa, Albert."
 */

#include "engine/systems/gravity_system.h"
#include "engine/components/components.h"
#include <math.h>

/* Distância mínima pra evitar singularidade (divisão por zero) */
#define MIN_DISTANCE 0.1

void bhs_gravity_system_central(bhs_world_handle world,
				struct bhs_vec3 center,
				real_t central_mass)
{
	if (!world)
		return;

	bhs_ecs_query q;
	bhs_ecs_query_init(&q, world, BHS_MASK_MOVABLE);

	bhs_entity_id id;
	while (bhs_ecs_query_next(&q, &id)) {
		bhs_transform_component *tr = bhs_ecs_get_component(
			world, id, BHS_COMP_TRANSFORM);
		bhs_physics_component *ph = bhs_ecs_get_component(
			world, id, BHS_COMP_PHYSICS);

		if (!tr || !ph)
			continue;

		/* Vetor do corpo pro centro */
		struct bhs_vec3 pos = {
			.x = tr->position.x,
			.y = tr->position.y,
			.z = tr->position.z
		};
		struct bhs_vec3 diff = bhs_vec3_sub(center, pos);
		real_t r = bhs_vec3_norm(diff);

		/* Evita singularidade */
		if (r < MIN_DISTANCE)
			continue;

		/*
		 * Força gravitacional: F = G * M * m / r²
		 * Aceleração: a = F/m = G * M / r²
		 * Força acumulada: F = m * a = G * M * m / r²
		 */
		real_t accel_mag = (BHS_G * central_mass) / (r * r);
		struct bhs_vec3 dir = bhs_vec3_normalize(diff);

		/* Acumula força (não aceleração!) */
		ph->force_accumulator.x += dir.x * accel_mag * ph->mass;
		ph->force_accumulator.y += dir.y * accel_mag * ph->mass;
		ph->force_accumulator.z += dir.z * accel_mag * ph->mass;
	}
}

void bhs_gravity_system_nbody(bhs_world_handle world)
{
	if (!world)
		return;

	/*
	 * N-Body: Cada corpo atrai todos os outros.
	 * Complexidade O(n²) - otimizações possíveis:
	 * - Barnes-Hut (octree) pra O(n log n)
	 * - GPU compute shader pra paralelização massiva
	 * 
	 * Por enquanto, força bruta. Funciona pra <1000 corpos.
	 */

	/* Primeiro, coletamos todas as entidades relevantes */
	bhs_ecs_query q;
	bhs_ecs_query_init_cached(&q, world, BHS_MASK_MOVABLE);

	/* Pra cada par de corpos */
	for (uint32_t i = 0; i < q.count; i++) {
		bhs_entity_id id_a = q.cache[i];
		bhs_transform_component *tr_a = bhs_ecs_get_component(
			world, id_a, BHS_COMP_TRANSFORM);
		bhs_physics_component *ph_a = bhs_ecs_get_component(
			world, id_a, BHS_COMP_PHYSICS);

		if (!tr_a || !ph_a)
			continue;

		struct bhs_vec3 pos_a = {
			.x = tr_a->position.x,
			.y = tr_a->position.y,
			.z = tr_a->position.z
		};

		for (uint32_t j = i + 1; j < q.count; j++) {
			bhs_entity_id id_b = q.cache[j];
			bhs_transform_component *tr_b = bhs_ecs_get_component(
				world, id_b, BHS_COMP_TRANSFORM);
			bhs_physics_component *ph_b = bhs_ecs_get_component(
				world, id_b, BHS_COMP_PHYSICS);

			if (!tr_b || !ph_b)
				continue;

			struct bhs_vec3 pos_b = {
				.x = tr_b->position.x,
				.y = tr_b->position.y,
				.z = tr_b->position.z
			};

			/* Vetor de A pra B */
			struct bhs_vec3 diff = bhs_vec3_sub(pos_b, pos_a);
			real_t r = bhs_vec3_norm(diff);

			if (r < MIN_DISTANCE)
				continue;

			/*
			 * Força entre A e B: F = G * m_a * m_b / r²
			 * Direção: A -> B pra força em A, B -> A pra força em B
			 */
			real_t force_mag = (BHS_G * ph_a->mass * ph_b->mass) / (r * r);
			struct bhs_vec3 dir = bhs_vec3_normalize(diff);

			/* A é atraído pra B */
			ph_a->force_accumulator.x += dir.x * force_mag;
			ph_a->force_accumulator.y += dir.y * force_mag;
			ph_a->force_accumulator.z += dir.z * force_mag;

			/* B é atraído pra A (Newton 3: ação = reação) */
			ph_b->force_accumulator.x -= dir.x * force_mag;
			ph_b->force_accumulator.y -= dir.y * force_mag;
			ph_b->force_accumulator.z -= dir.z * force_mag;
		}
	}

	bhs_ecs_query_destroy(&q);
}
