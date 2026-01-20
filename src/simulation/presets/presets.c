/**
 * @file presets.c
 * @brief Implementação dos Corpos Celestes Pré-Definidos
 *
 * "Criar um Sol é fácil. Manter os planetas em órbita é a parte difícil."
 *
 * Este arquivo usa o sistema de unidades definido em lib/units.h.
 * Todas as proporções físicas são preservadas.
 */

#include "presets.h"
#include "../data/planet.h"
#include "math/units.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * FUNÇÕES AUXILIARES
 * ============================================================================
 */

double bhs_preset_orbital_velocity(double central_mass, double orbital_radius)
{
	/* Usa a função do sistema de unidades */
	return bhs_orbital_velocity(central_mass, orbital_radius);
}

/**
 * create_body_from_module - Converte descritor de planeta em corpo simulável
 * @desc: Descritor com dados SI reais (de planets/)
 * @center_pos: Posição do corpo central (Sol)
 * @central_mass_sim: Massa do corpo central em unidades de simulação
 *
 * Aplica escalas para converter unidades SI em unidades de simulação.
 * 
 * ESCALAS (definidas no topo do arquivo):
 *   MASS:   1e29 kg → 1.0 unidade
 *   DIST:   1 AU    → 50 unidades
 *   RADIUS: R_sol   → 3.0 unidades
 *
 * PROPORÇÕES REAIS DOS RAIOS:
 *   Sol:     696,340 km → 3.00 unidades
 *   Júpiter:  69,911 km → 0.30 unidades (10x menor que Sol)
 *   Saturno:  58,232 km → 0.25 unidades
 *   Terra:     6,371 km → 0.027 unidades (109x menor que Sol)
 *   Mercúrio:  2,439 km → 0.011 unidades
 *
 * Usamos os valores REAIS sem modificação.
 */
static struct bhs_body create_body_from_module(struct bhs_planet_desc desc, 
					       struct bhs_vec3 center_pos, 
					       double central_mass_sim)
{
	/* Posição: distância orbital convertida para simulação */
	double r = BHS_METERS_TO_SIM(desc.semimajor_axis);
	
	struct bhs_vec3 pos = {
		center_pos.x + r,
		center_pos.y,
		center_pos.z
	};
	
	/* Cria corpo base a partir do descritor */
	struct bhs_body b = bhs_body_create_from_desc(&desc, pos);
	
	/* Aplica escalas de massa e raio - SEM OVERRIDES */
	b.state.mass = BHS_KG_TO_SIM(b.state.mass);
	b.state.radius = BHS_RADIUS_TO_SIM(b.state.radius);
	
	printf("[PRESET] %s: M=%.2e, R=%.4f (real), Dist=%.1f\n", 
		desc.name, b.state.mass, b.state.radius, r);

	/* Velocidade orbital: v = sqrt(G × M_central / r) */
	if (central_mass_sim > 0.0 && r > 0.0) {
		double v = bhs_preset_orbital_velocity(central_mass_sim, r);
		/* Órbita no plano XZ (Y é "para cima") */
		b.state.vel = (struct bhs_vec3){ 0, 0, v };
	}
		
	return b;
}

/* ============================================================================
 * MAIN PRESET LOADER
 * ============================================================================
 */

void bhs_preset_solar_system(bhs_scene_t scene)
{
	if (!scene) {
		fprintf(stderr, "[PRESET] Erro: cena nula\n");
		return;
	}

	printf("[PRESET] Criando Sistema Solar Completo...\n");
	printf("[PRESET] Sistema de unidades: lib/units.h (G=1, M☉=20, R☉=3, AU=50)\n");

	/* 1. SUN - Dados vindos de sun.c */
	struct bhs_planet_desc d_sun = bhs_sun_get_desc();
	
	printf("[PRESET] Sol (REAL): M=%.3e kg, R=%.3e m\n", 
		d_sun.mass, d_sun.radius);
	
	struct bhs_body sun = bhs_body_create_from_desc(&d_sun, (struct bhs_vec3){0,0,0});
	
	/* Aplica escalas usando units.h - SEM overrides hardcoded */
	sun.state.mass = BHS_KG_TO_SIM(sun.state.mass);
	sun.state.radius = BHS_RADIUS_TO_SIM(sun.state.radius);
	
	printf("[PRESET] Sol (SIM):  M=%.2f, R=%.2f\n", 
		sun.state.mass, sun.state.radius);
	fflush(stdout);
	
	/* Sol é fixo no centro */
	sun.is_fixed = true;
	
	bhs_scene_add_body_struct(scene, sun);

	double M_sun = sun.state.mass;

	/* 2. PLANETS */
	struct bhs_planet_desc (*planet_getters[])(void) = {
		bhs_mercury_get_desc,
		bhs_venus_get_desc,
		bhs_earth_get_desc,
		bhs_mars_get_desc,
		bhs_jupiter_get_desc,
		bhs_saturn_get_desc,
		bhs_uranus_get_desc,
		bhs_neptune_get_desc,
		bhs_pluto_get_desc, /* Pluto acts as generic dwarf here */
		NULL
	};

	for (int i = 0; planet_getters[i] != NULL; i++) {
		struct bhs_planet_desc d = planet_getters[i]();
		struct bhs_body b = create_body_from_module(d, sun.state.pos, M_sun);
		bhs_scene_add_body_struct(scene, b);
	}

	printf("[PRESET] Sistema Solar Completo Carregado!\n");
}

void bhs_preset_earth_sun_only(bhs_scene_t scene)
{
	if (!scene) return;

	printf("[PRESET] Criando APENAS Sol e Terra (Escala Real)...\n");

	/* 1. SUN */
	struct bhs_planet_desc d_sun = bhs_sun_get_desc();
	struct bhs_body sun = bhs_body_create_from_desc(&d_sun, (struct bhs_vec3){0,0,0});
	
	/* Escalas */
	sun.state.mass = BHS_KG_TO_SIM(sun.state.mass);
	sun.state.radius = BHS_RADIUS_TO_SIM(sun.state.radius);
	sun.is_fixed = true;
	
	bhs_scene_add_body_struct(scene, sun);

	/* 2. EARTH */
	struct bhs_planet_desc d_earth = bhs_earth_get_desc();
	struct bhs_body earth = create_body_from_module(d_earth, sun.state.pos, sun.state.mass);
	
	bhs_scene_add_body_struct(scene, earth);

	printf("[PRESET] Sol e Terra carregados.\n");
}

/* Backward compatibility dummies if needed, but we replaced the main loop */
struct bhs_body bhs_preset_sun(struct bhs_vec3 pos) {
	/* Não usado pelo main loop, mas mantido para compatibilidade */
	struct bhs_planet_desc d = bhs_sun_get_desc();
	struct bhs_body b = bhs_body_create_from_desc(&d, pos);
	b.state.mass = BHS_KG_TO_SIM(b.state.mass);
	b.state.radius = BHS_RADIUS_TO_SIM(b.state.radius);
	return b;
}

struct bhs_body bhs_preset_earth(struct bhs_vec3 sun_pos) {
	struct bhs_planet_desc d = bhs_earth_get_desc();
	return create_body_from_module(d, sun_pos, BHS_SIM_MASS_SUN);
}

struct bhs_body bhs_preset_moon(struct bhs_vec3 earth_pos, struct bhs_vec3 earth_vel) {
	(void)earth_pos;
	(void)earth_vel;
	// Moon is special, relative to Earth
	// Implementing Moon via module would require specific handling, 
	// for now we can approximate or skip or custom impl.
	// Since the user asked for "Sol, Planetas, Buraco Negro", Moon is secondary.
	// But let's keep the function signature valid.
	struct bhs_body m = {0};
	// Simplified return empty or custom logic
    return m;
}
