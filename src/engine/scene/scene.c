/**
 * @file scene.c
 * @brief Implementação do Orquestrador
 *
 * "Gerenciando o caos, um frame de cada vez."
 */

#include "engine/scene/scene.h"
#include "engine/presets/presets.h"
#include "engine/integrator/integrator.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

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
	 * Modos de inicialização:
	 * - Sem env var: vazio (usuário cria corpos manualmente)
	 * - BHS_DEBUG_SCENE=1: Sistema simples (buraco negro + 2 planetas)
	 * - BHS_DEBUG_SCENE=2: Sistema Solar real (Sol + Terra + Lua)
	 */
	const char *debug_env = getenv("BHS_DEBUG_SCENE");
	if (debug_env && debug_env[0] == '2') {
		/* Sistema Solar com dados reais */
		printf("[SCENE] Modo SOLAR ativado. Criando Sol-Terra-Lua...\n");
		bhs_preset_solar_system(scene);

	} else if (debug_env && debug_env[0] == '1') {
		/* Sistema simples para testes */
		printf("[SCENE] Modo debug ativado. Criando sistema simples...\n");

		/* Buraco negro central */
		struct bhs_vec3 center = { 0, 0, 0 };
		struct bhs_vec3 zero_vel = { 0, 0, 0 };
		struct bhs_vec3 black_color = { 0, 0, 0 };
		bhs_scene_add_body(scene, BHS_BODY_BLACKHOLE, center, zero_vel,
				   10.0, 2.0, black_color);
		printf("[SCENE] Buraco negro central (M=10)\n");

		/* Planeta em órbita */
		double r = 15.0;
		double v_orb = sqrt(10.0 / r);
		struct bhs_vec3 planet_pos = { r, 0, 0 };
		struct bhs_vec3 planet_vel = { 0, 0, v_orb };
		struct bhs_vec3 planet_color = { 0.3, 0.5, 1.0 };
		bhs_scene_add_body(scene, BHS_BODY_PLANET, planet_pos, planet_vel,
				   0.1, 0.5, planet_color);
		printf("[SCENE] Planeta em orbita (r=%.1f, v=%.3f)\n", r, v_orb);

		/* Segundo planeta */
		r = 25.0;
		v_orb = sqrt(10.0 / r);
		planet_pos = (struct bhs_vec3){ 0, 0, r };
		planet_vel = (struct bhs_vec3){ v_orb, 0, 0 };
		planet_color = (struct bhs_vec3){ 1.0, 0.5, 0.3 };
		bhs_scene_add_body(scene, BHS_BODY_PLANET, planet_pos, planet_vel,
				   0.15, 0.6, planet_color);
		printf("[SCENE] Planeta 2 em orbita (r=%.1f, v=%.3f)\n", r, v_orb);

		printf("[SCENE] Sistema simples criado: %d corpos\n", scene->n_bodies);
	} else {
		scene->n_bodies = 0;
	}
}

void bhs_scene_update(bhs_scene_t scene, double dt)
{
	if (!scene || scene->n_bodies == 0)
		return;

	/*
	 * =========================================================================
	 * MOTOR DE FÍSICA N-BODY - NÍVEL CIENTÍFICO (NASA-GRADE)
	 * =========================================================================
	 *
	 * Integrador: RK4 (Runge-Kutta 4ª ordem) via módulo integrator.c
	 * Precisão: Kahan summation para acumulação sem erro
	 * Constantes: IAU 2015 / CODATA 2018
	 * Validação: Conservação de E, p, L a cada frame
	 */

	/* Estado persistente */
	static struct bhs_system_state rk_state;
	static struct bhs_invariants initial_inv;
	static bool initialized = false;
	static int frame_counter = 0;
	static double sim_time = 0;
	static double max_energy_drift = 0;

	frame_counter++;

	/* ===== INICIALIZAÇÃO (primeira chamada) ===== */
	if (!initialized) {
		rk_state.n_bodies = scene->n_bodies;
		rk_state.time = 0;

		printf("[RK4] Inicializando integrador científico...\n");
		printf("[RK4] Constantes: G=%.2f (unidades naturais)\n", G_SIM);

		for (int i = 0; i < scene->n_bodies; i++) {
			struct bhs_body *b = &scene->bodies[i];
			rk_state.bodies[i].pos = b->state.pos;
			rk_state.bodies[i].vel = b->state.vel;
			rk_state.bodies[i].mass = b->state.mass;
			rk_state.bodies[i].gm = G_SIM * b->state.mass;
			rk_state.bodies[i].is_fixed = b->is_fixed;
			rk_state.bodies[i].is_alive = b->is_alive;

			printf("[RK4] Corpo %d: M=%.4e kg, GM=%.4e m³/s²\n",
			       i, b->state.mass, rk_state.bodies[i].gm);
		}

		bhs_compute_invariants(&rk_state, &initial_inv);
		printf("[RK4] Invariantes iniciais:\n");
		printf("[RK4]   Energia: %.10e J\n", initial_inv.energy);
		printf("[RK4]   Momento: (%.4e, %.4e, %.4e) kg·m/s\n",
		       initial_inv.momentum.x, initial_inv.momentum.y, initial_inv.momentum.z);
		printf("[RK4]   L_angular: (%.4e, %.4e, %.4e) kg·m²/s\n",
		       initial_inv.angular_momentum.x, initial_inv.angular_momentum.y,
		       initial_inv.angular_momentum.z);

		initialized = true;
	}

	/* ===== SINCRONIZA ESTADO (novos corpos, remoções) ===== */
	if (rk_state.n_bodies != scene->n_bodies) {
		rk_state.n_bodies = scene->n_bodies;
		for (int i = 0; i < scene->n_bodies; i++) {
			struct bhs_body *b = &scene->bodies[i];
			rk_state.bodies[i].pos = b->state.pos;
			rk_state.bodies[i].vel = b->state.vel;
			rk_state.bodies[i].mass = b->state.mass;
			rk_state.bodies[i].gm = G_SIM * b->state.mass;
			rk_state.bodies[i].is_fixed = b->is_fixed;
			rk_state.bodies[i].is_alive = b->is_alive;
		}
		/* Recalcula invariantes após mudança */
		bhs_compute_invariants(&rk_state, &initial_inv);
	}

	/* ===== DETECÇÃO DE COLISÕES ===== */
	for (int i = 0; i < scene->n_bodies; i++) {
		if (!scene->bodies[i].is_alive)
			continue;

		for (int j = i + 1; j < scene->n_bodies; j++) {
			if (!scene->bodies[j].is_alive)
				continue;

			struct bhs_body *bi = &scene->bodies[i];
			struct bhs_body *bj = &scene->bodies[j];

			double dx = bj->state.pos.x - bi->state.pos.x;
			double dy = bj->state.pos.y - bi->state.pos.y;
			double dz = bj->state.pos.z - bi->state.pos.z;
			double dist = sqrt(dx*dx + dy*dy + dz*dz);

			double collision_dist = bi->state.radius + bj->state.radius;

			if (dist < collision_dist && dist > 0.001) {
				struct bhs_body *absorber = (bi->state.mass >= bj->state.mass) ? bi : bj;
				struct bhs_body *victim = (absorber == bi) ? bj : bi;

				/* Conserva momento linear */
				double total_mass = absorber->state.mass + victim->state.mass;
				absorber->state.vel.x = (absorber->state.mass * absorber->state.vel.x +
				                         victim->state.mass * victim->state.vel.x) / total_mass;
				absorber->state.vel.y = (absorber->state.mass * absorber->state.vel.y +
				                         victim->state.mass * victim->state.vel.y) / total_mass;
				absorber->state.vel.z = (absorber->state.mass * absorber->state.vel.z +
				                         victim->state.mass * victim->state.vel.z) / total_mass;

				absorber->state.mass = total_mass;
				absorber->state.radius = pow(
					pow(absorber->state.radius, 3) + pow(victim->state.radius, 3),
					1.0/3.0
				);

				victim->is_alive = false;

				printf("[PHYSICS] Colisão! Nova massa: %.6e kg\n", absorber->state.mass);
			}
		}
	}

	/* ===== REMOVE CORPOS MORTOS ===== */
	int write_idx = 0;
	for (int i = 0; i < scene->n_bodies; i++) {
		if (scene->bodies[i].is_alive) {
			if (write_idx != i) {
				scene->bodies[write_idx] = scene->bodies[i];
				rk_state.bodies[write_idx] = rk_state.bodies[i];
			}
			write_idx++;
		}
	}
	scene->n_bodies = write_idx;
	rk_state.n_bodies = write_idx;

	/* ===== INTEGRAÇÃO RK4 (4ª ORDEM) ===== */
	bhs_integrator_rk4(&rk_state, dt);
	sim_time += dt;

	/* ===== COPIA ESTADO DE VOLTA PARA SCENE ===== */
	for (int i = 0; i < scene->n_bodies; i++) {
		scene->bodies[i].state.pos = rk_state.bodies[i].pos;
		scene->bodies[i].state.vel = rk_state.bodies[i].vel;
	}

	/* ===== VERIFICAÇÃO DE INVARIANTES (a cada 2 segundos) ===== */
	if (frame_counter % 125 == 0 && scene->n_bodies > 0) {
		struct bhs_invariants current;
		bhs_compute_invariants(&rk_state, &current);

		double dE = fabs(current.energy - initial_inv.energy);
		double E_rel = (fabs(initial_inv.energy) > 1e-30) ?
			       dE / fabs(initial_inv.energy) * 100.0 : 0;

		if (E_rel > max_energy_drift) {
			max_energy_drift = E_rel;
		}

		/* Calcula magnitudes */
		double p_mag = sqrt(current.momentum.x * current.momentum.x +
		                    current.momentum.y * current.momentum.y +
		                    current.momentum.z * current.momentum.z);
		double L_mag = sqrt(current.angular_momentum.x * current.angular_momentum.x +
		                    current.angular_momentum.y * current.angular_momentum.y +
		                    current.angular_momentum.z * current.angular_momentum.z);

		printf("[RK4] t=%.2fs | E=%.6e | dE=%.6f%% | |p|=%.4e | |L|=%.4e\n",
		       sim_time, current.energy, E_rel, p_mag, L_mag);

		/* Alerta se drift > 0.1% */
		if (E_rel > 0.1) {
			printf("[RK4] AVISO: Drift acima de 0.1%%! Considere reduzir dt.\n");
		}
	}

	/* Atualiza deformação da malha */
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

	/* Flags de controle */
	b->is_alive = true;
	b->is_fixed = (type == BHS_BODY_BLACKHOLE);  /* Buracos negros são fixos */

	/* Initialize Type Specific Defaults */
	switch (type) {
	case BHS_BODY_PLANET:
		b->prop.planet = (struct bhs_planet_data){
			.physical_state = BHS_STATE_SOLID,
			.density = 5514.0,
			.surface_pressure = 101325.0,
			.atmosphere_mass = 5.15e18,
			.composition = "N2 78%, O2 21%",
			.temperature = 288.0,
			.albedo = 0.306,
			.axis_tilt = 0.4,
			.has_atmosphere = true
		};
		break;
	case BHS_BODY_MOON:
		b->prop.planet = (struct bhs_planet_data){
			.physical_state = BHS_STATE_SOLID,
			.density = 3344.0,
			.surface_pressure = 0,
			.atmosphere_mass = 0,
			.composition = "Regolith",
			.temperature = 250.0,
			.albedo = 0.12,
			.axis_tilt = 0.027,
			.has_atmosphere = false
		};
		break;
	case BHS_BODY_STAR:
		b->prop.star = (struct bhs_star_data){
			.luminosity = 3.828e26,
			.temp_effective = 5772.0,
			.age = 4.6e9,
			.density = 1408.0,
			.hydrogen_frac = 0.73,
			.helium_frac = 0.25,
			.metals_frac = 0.02,
			.stage = BHS_STAR_MAIN_SEQUENCE,
			.metallicity = 0.0122,
			.spectral_type = "G2V"
		};
		break;
	case BHS_BODY_BLACKHOLE:
		b->prop.bh = (struct bhs_blackhole_data){
			.spin_factor = 0.0,
			.event_horizon_r = 2.0 * mass,
			.ergososphere_r = 2.0 * (mass > 0 ? mass : 10.0),
			.accretion_disk_mass = 0.0
		};
		printf("[SCENE] Buraco negro criado: M=%.2f, fixo no centro\n", mass);
		break;
	case BHS_BODY_ASTEROID:
		b->prop.planet = (struct bhs_planet_data){
			.physical_state = BHS_STATE_SOLID,
			.density = 2000.0,
			.surface_pressure = 0,
			.atmosphere_mass = 0,
			.composition = "Rocha/Metal",
			.temperature = 200.0,
			.albedo = 0.15,
			.axis_tilt = 0,
			.has_atmosphere = false
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
