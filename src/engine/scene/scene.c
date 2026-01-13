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

bhs_scene_t bhs_scene_create(void)
{
	bhs_scene_t scene = calloc(1, sizeof(struct bhs_scene_impl));
	if (!scene)
		return NULL;

	scene->n_bodies = 0;
	scene->spacetime = NULL;

	return scene;
}

void bhs_scene_destroy(bhs_scene_t scene)
{
	if (!scene)
		return;

	if (scene->spacetime) {
		bhs_spacetime_destroy(scene->spacetime);
	}

	free(scene);
}

void bhs_scene_init_default(bhs_scene_t scene)
{
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

	/*
	 * [USER REQUEST] Start empty.
	 * "Ao entrar, não há malha espacial, não há estrelas nem buracos negros nem nada."
	 */
	scene->n_bodies = 0;

	/*
	 * Nota: O array bodies[] já está alocado estaticamente no struct,
	 * então apenas zerar o contador é suficiente.
	 */
}

void bhs_scene_update(bhs_scene_t scene, double dt)
{
	if (!scene)
		return;

	/* 1. Integração de movimento dos corpos */
	/*
   * TODO: Implementar gravidade N-Corpos real.
   * Por enquanto, apenas move linearmente ou orbita fixa em torno da origem
   * (Assumindo que corpo 0 é estático na origem)
   */
	/* 1. Integração N-Corpos (Velocity Verlet) */
	/*
	 * Algoritmo:
	 * 1. v(t + 0.5*dt) = v(t) + 0.5 * a(t) * dt
	 * 2. r(t + dt)     = r(t) + v(t + 0.5*dt) * dt
	 * 3. a(t + dt)     = Forces(r(t + dt))
	 * 4. v(t + dt)     = v(t + 0.5*dt) + 0.5 * a(t + dt) * dt
	 *
	 * Simplificado para implementação direta:
	 * 1. r_new = r + v*dt + 0.5*a*dt*dt
	 * 2. v_half = v + 0.5*a*dt
	 * 3. a_new = Forces(r_new)
	 * 4. v_new = v_half + 0.5*a_new*dt
	 */

	/* Passo 1 e 2: Atualiza Posição e Meia-Velocidade */
	/* Nota: Assume que 'b->state.acc' contém a aceleração do frame anterior */
	for (int i = 0; i < scene->n_bodies; i++) {
		struct bhs_body *b = &scene->bodies[i];

		/* r(t+dt) = r(t) + v(t)*dt + 0.5*a(t)*dt^2 */
		double dt2_half = 0.5 * dt * dt;
		b->state.pos.x += b->state.vel.x * dt + b->state.acc.x * dt2_half;
		b->state.pos.y += b->state.vel.y * dt + b->state.acc.y * dt2_half;
		b->state.pos.z += b->state.vel.z * dt + b->state.acc.z * dt2_half;

		/* v_half = v(t) + 0.5*a(t)*dt */
		double dt_half = 0.5 * dt;
		b->state.vel.x += b->state.acc.x * dt_half;
		b->state.vel.y += b->state.acc.y * dt_half;
		b->state.vel.z += b->state.acc.z * dt_half;
	}

	/* Passo 3: Recalcula Forças (Aceleração) com Novas Posições */
	/* Zera acelerações */
	for (int i = 0; i < scene->n_bodies; i++) {
		scene->bodies[i].state.acc = (struct bhs_vec3){ 0, 0, 0 };
	}

	/* Acumula Força Mútua (N^2 / 2) */
	for (int i = 0; i < scene->n_bodies; i++) {
		for (int j = i + 1; j < scene->n_bodies; j++) {
			struct bhs_body *bi = &scene->bodies[i];
			struct bhs_body *bj = &scene->bodies[j];

			double dx = bj->state.pos.x - bi->state.pos.x;
			double dy = bj->state.pos.y - bi->state.pos.y;
			double dz = bj->state.pos.z - bi->state.pos.z;

			double dist_sq = dx * dx + dy * dy + dz * dz;
			double dist = sqrt(dist_sq);

			/* Softening para evitar singularidade */
			if (dist < 0.1)
				dist = 0.1;

			/* F = G * m1 * m2 / r^2 */
			/* F_vec = F * (dir) = (G * m1 * m2 / r^2) * (d / r) */
			/* F_vec = G * m1 * m2 * d / r^3 */
			double r3 = dist * dist * dist;
			double G = 1.0; /* Unidade natural (Geometrized Units) para realismo relativístico */
			double factor = G * bi->state.mass * bj->state.mass / r3;

			double fx = factor * dx;
			double fy = factor * dy;
			double fz = factor * dz;

			/* Aplica leis de Newton (Ação e Reação) */
			/* a = F/m */
			bi->state.acc.x += fx / bi->state.mass;
			bi->state.acc.y += fy / bi->state.mass;
			bi->state.acc.z += fz / bi->state.mass;

			bj->state.acc.x -= fx / bj->state.mass;
			bj->state.acc.y -= fy / bj->state.mass;
			bj->state.acc.z -= fz / bj->state.mass;
		}
	}

	/* Passo 4: Completa Velocidade */
	for (int i = 0; i < scene->n_bodies; i++) {
		struct bhs_body *b = &scene->bodies[i];
		double dt_half = 0.5 * dt;
		b->state.vel.x += b->state.acc.x * dt_half;
		b->state.vel.y += b->state.acc.y * dt_half;
		b->state.vel.z += b->state.acc.z * dt_half;
	}

	/* 2. Atualiza deformação da malha */
	if (scene->spacetime) {
		bhs_spacetime_update(scene->spacetime, scene->bodies,
				     scene->n_bodies);
	}
}

bhs_spacetime_t bhs_scene_get_spacetime(bhs_scene_t scene)
{
	return scene ? scene->spacetime : NULL;
}

const struct bhs_body *bhs_scene_get_bodies(bhs_scene_t scene, int *count)
{
	if (!scene) {
		*count = 0;
		return NULL;
	}
	*count = scene->n_bodies;
	return scene->bodies;
}

bool bhs_scene_add_body(bhs_scene_t scene, enum bhs_body_type type,
			struct bhs_vec3 pos, struct bhs_vec3 vel, double mass,
			double radius, struct bhs_vec3 color)
{
	if (!scene || scene->n_bodies >= MAX_BODIES)
		return false;

	struct bhs_body *b = &scene->bodies[scene->n_bodies];

	/* Initialize State */
	b->state.pos = pos;
	b->state.vel = vel;
	b->state.acc = (struct bhs_vec3){ 0, 0, 0 };
	b->state.mass = mass;
	b->state.radius = radius;
	b->state.rot_axis = (struct bhs_vec3){ 0, 1, 0 };
	b->state.rot_speed = 0.0;

	b->type = type;
	b->color = color;

	/* Initialize Type Specific Defaults (Zero initialization for now) */
	/* Real values should be passed in via specific create functions in future */
	/* Initialize Type Specific Defaults (Zero initialization for now) */
	switch (type) {
	case BHS_BODY_PLANET:
		b->prop.planet = (struct bhs_planet_data){
			.physical_state = BHS_STATE_SOLID,
			.density = 5514.0,
			.surface_pressure = 1.0,
			.atmosphere_mass = 5.1e18,
			.composition = "N2 78%, O2 21%",
			.temperature = 288.0,
			.albedo = 0.3,
			.axis_tilt = 0.4,
			.has_atmosphere = true
		};
		break;
	case BHS_BODY_STAR:
		b->prop.star = (struct bhs_star_data){
			.luminosity = 3.8e26,
			.temp_effective = 5778.0,
			.age = 4.6e9,
			.stage = BHS_STAR_MAIN_SEQUENCE,
			.metallicity = 0.012,
			.spectral_type = "G2V"
		};
		break;
	case BHS_BODY_BLACKHOLE:
		b->prop.bh = (struct bhs_blackhole_data){
			.spin_factor = 0.0,
			.event_horizon_r = 2.0 * mass,
			.ergososphere_r = 2.0 * (mass > 0 ? mass : 10.0), // Fallback
			.accretion_disk_mass = 0.0
		};
		break;
	}

	scene->n_bodies++;
	return true;
}

void bhs_scene_remove_body(bhs_scene_t scene, int index)
{
	if (!scene || index < 0 || index >= scene->n_bodies)
		return;

	/* Shift left */
	for (int i = index; i < scene->n_bodies - 1; i++) {
		scene->bodies[i] = scene->bodies[i + 1];
	}

	scene->n_bodies--;
}
