/**
 * @file physics_system.c
 * @brief Integrador de Física (CPU)
 *
 * "Euler semi-implícito: simples, estável, e bom o suficiente pra órbitas."
 *
 * NOTA: Este sistema APENAS integra posição/velocidade.
 * Gravidade e outras forças são aplicadas por sistemas separados
 * (gravity_system.c) ANTES deste rodar.
 */

#include "engine/systems/physics_system.h"
#include "engine/components/components.h"

void bhs_physics_system_update(bhs_world_handle world, real_t dt)
{
	if (!world)
		return;

	/*
	 * Query otimizada: só itera entidades com Transform + Physics.
	 * Adeus, loop de 10000 entidades. Linus ficaria orgulhoso.
	 */
	bhs_ecs_query q;
	bhs_ecs_query_init(&q, world, BHS_MASK_MOVABLE);

	bhs_entity_id id;
	while (bhs_ecs_query_next(&q, &id)) {
		bhs_transform_component *tr = bhs_ecs_get_component(
			world, id, BHS_COMP_TRANSFORM);
		bhs_physics_component *ph = bhs_ecs_get_component(
			world, id, BHS_COMP_PHYSICS);

		/*
		 * Integração Semi-Implícita de Euler (Symplectic)
		 *
		 * Ordem:
		 * 1. v(t+dt) = v(t) + a(t)*dt
		 * 2. x(t+dt) = x(t) + v(t+dt)*dt
		 *
		 * Importante: usa velocidade NOVA pra atualizar posição.
		 * Isso conserva energia melhor que Euler explícito.
		 */

		/* 1. Calcula aceleração: a = F/m */
		struct bhs_vec4 accel = bhs_vec4_scale(
			ph->force_accumulator, ph->inverse_mass);

		/* 2. Atualiza velocidade: v += a*dt */
		ph->velocity = bhs_vec4_add(
			ph->velocity, bhs_vec4_scale(accel, dt));

		/* 3. Atualiza posição: x += v*dt (com v já atualizado) */
		tr->position = bhs_vec4_add(
			tr->position, bhs_vec4_scale(ph->velocity, dt));

		/* 4. Limpa forças acumuladas pro próximo frame */
		ph->force_accumulator = bhs_vec4_zero();

		/* 5. Aplica drag (resistência) */
		if (ph->drag > 0) {
			real_t damping = 1.0 - (ph->drag * dt);
			if (damping < 0) damping = 0;
			ph->velocity = bhs_vec4_scale(ph->velocity, damping);
		}
	}
}

