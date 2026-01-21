/**
 * @file scenario_mgr.c
 * @brief Implementação do Gerenciador de Cenários
 *
 * "Cada cenário é um universo. E cada universo precisa de um Deus.
 *  Nesse caso, o Deus é você. Parabéns pela responsabilidade."
 */

#include "scenario_mgr.h"
#include "src/app_state.h"
#include "src/simulation/presets/presets.h"

#include "engine/scene/scene.h"
#include "gui/log.h"
#include "math/vec4.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* ============================================================================
 * HELPERS
 * ============================================================================
 */

/**
 * Reposiciona câmera para visualização inicial do cenário
 */
static void set_camera_for_scenario(struct app_state *app, enum scenario_type type)
{
	if (!app)
		return;

	switch (type) {
	case SCENARIO_SOLAR_SYSTEM:
		/* Câmera acima do plano orbital, olhando pro centro */
		app->camera.x = 0.0f;
		app->camera.y = 80.0f;
		app->camera.z = -120.0f;
		app->camera.pitch = -0.4f;
		app->camera.yaw = 0.0f;
		app->camera.fov = 600.0f;
		break;

	case SCENARIO_EARTH_SUN:
		/* Câmera focada em observar a distância */
		app->camera.x = 0.0f;
		app->camera.y = 100.0f; /* Bem acima */
		app->camera.z = 0.0f;
		app->camera.pitch = -1.57f; /* Olhando para baixo (90 graus) */
		app->camera.yaw = 0.0f;
		app->camera.fov = 1000.0f;
		break;

	case SCENARIO_KERR_BLACKHOLE:
		/* Perto do evento horizon */
		app->camera.x = 15.0f;
		app->camera.y = 5.0f;
		app->camera.z = -20.0f;
		app->camera.pitch = -0.1f;
		app->camera.yaw = 0.0f;
		app->camera.fov = 500.0f;
		break;

	case SCENARIO_BINARY_STAR:
		/* Vista lateral do sistema binário */
		app->camera.x = 0.0f;
		app->camera.y = 30.0f;
		app->camera.z = -60.0f;
		app->camera.pitch = -0.3f;
		app->camera.yaw = 0.0f;
		app->camera.fov = 500.0f;
		break;

	case SCENARIO_DEBUG:
	default:
		/* Posição debug padrão */
		app->camera.x = 0.0f;
		app->camera.y = 20.0f;
		app->camera.z = -40.0f;
		app->camera.pitch = -0.3f;
		app->camera.yaw = 0.0f;
		app->camera.fov = 500.0f;
		break;
	}
}

/* ============================================================================
 * LOADERS DE CENÁRIO
 * ============================================================================
 */

static bool load_solar_system(struct app_state *app)
{
	BHS_LOG_INFO("Carregando Sistema Solar...");
	bhs_preset_solar_system(app->scene);
	return true;
}

static bool load_earth_sun(struct app_state *app)
{
	BHS_LOG_INFO("Carregando Sol e Terra...");
	bhs_preset_earth_sun_only(app->scene);
	return true;
}

static bool load_kerr_blackhole(struct app_state *app)
{
	BHS_LOG_INFO("Carregando Black Hole Kerr...");

	struct bhs_vec3 center = { 0, 0, 0 };
	struct bhs_vec3 zero = { 0, 0, 0 };
	struct bhs_vec3 black = { 0, 0, 0 };

	/* Black hole central */
	bhs_scene_add_body(app->scene, BHS_BODY_BLACKHOLE, center, zero,
			   10.0, 2.0, black);

	/* Partículas em órbita */
	for (int i = 0; i < 8; i++) {
		double angle = (double)i * 3.14159 / 4.0;
		double r = 15.0 + (double)i * 3.0;
		double v = sqrt(10.0 / r);

		struct bhs_vec3 pos = {
			.x = r * cos(angle),
			.y = 0.0,
			.z = r * sin(angle)
		};
		struct bhs_vec3 vel = {
			.x = -v * sin(angle),
			.y = 0.0,
			.z = v * cos(angle)
		};
		struct bhs_vec3 col = {
			.x = 0.3f + 0.1f * i,
			.y = 0.5f,
			.z = 1.0f - 0.1f * i
		};

		bhs_scene_add_body(app->scene, BHS_BODY_PLANET, pos, vel,
				   0.1, 0.5, col);
	}

	return true;
}

static bool load_binary_star(struct app_state *app)
{
	BHS_LOG_INFO("Carregando Sistema Binário...");

	/* Duas estrelas orbitando centro de massa */
	double separation = 20.0;
	double m1 = 5.0, m2 = 3.0;
	double total_m = m1 + m2;
	double r1 = separation * m2 / total_m;
	double r2 = separation * m1 / total_m;
	double v_orb = sqrt(total_m / separation);

	struct bhs_vec3 yellow = { 1.0, 0.9, 0.3 };
	struct bhs_vec3 orange = { 1.0, 0.5, 0.2 };

	/* Estrela 1 */
	bhs_scene_add_body(app->scene, BHS_BODY_STAR,
		(struct bhs_vec3){ -r1, 0, 0 },
		(struct bhs_vec3){ 0, 0, v_orb * r1 / separation },
		m1, 2.0, yellow);

	/* Estrela 2 */
	bhs_scene_add_body(app->scene, BHS_BODY_STAR,
		(struct bhs_vec3){ r2, 0, 0 },
		(struct bhs_vec3){ 0, 0, -v_orb * r2 / separation },
		m2, 1.5, orange);

	/* Planeta orbitando o sistema */
	double r_planet = 50.0;
	double v_planet = sqrt(total_m / r_planet);
	bhs_scene_add_body(app->scene, BHS_BODY_PLANET,
		(struct bhs_vec3){ r_planet, 0, 0 },
		(struct bhs_vec3){ 0, 0, v_planet },
		0.1, 0.4, (struct bhs_vec3){ 0.3, 0.5, 1.0 });

	return true;
}

static bool load_debug(struct app_state *app)
{
	BHS_LOG_INFO("Carregando cenário de debug (SINGLE PLANET)...");

	/* Single Planet at (30, 0, 0) */
	/* Radius 1.0 (will be scaled x30 visually = 30.0) */
	/* Should be HUGE on screen if camera is at (0, 20, -40) looking at (0,0,0) */
	/* Wait, (30, 0, 0) is to the RIGHT. */
	
	/* Let's put it DEAD CENTER 0,0,0 to be sure */
	bhs_scene_add_body(app->scene, BHS_BODY_PLANET,
		(struct bhs_vec3){ 0, 0, 0 },     /* Pos 0,0,0 */
		(struct bhs_vec3){ 0, 0, 0 },     /* Static */
		1.0, 1.0,                         /* Mass, Radius=1.0 */
		(struct bhs_vec3){ 1.0, 0.0, 1.0 } /* Magenta Color */
	);

	/* REFERENCE PLANET (Blue) at (20, 0, 0) */
	bhs_scene_add_body(app->scene, BHS_BODY_PLANET,
		(struct bhs_vec3){ 30.0, 0, 0 },     /* Pos 30,0,0 */
		(struct bhs_vec3){ 0, 0, 0 },     /* Static */
		1.0, 1.0,                         /* Mass, Radius=1.0 */
		(struct bhs_vec3){ 0.0, 1.0, 1.0 } /* Cyan Color */
	);

	return true;
}

/* ============================================================================
 * API PÚBLICA
 * ============================================================================
 */

bool scenario_load(struct app_state *app, enum scenario_type type)
{
	if (!app || !app->scene) {
		BHS_LOG_ERROR("scenario_load: app ou scene nulos");
		return false;
	}

	/* Limpa cenário anterior */
	scenario_unload(app);

	/* Carrega o novo */
	bool ok = false;

	switch (type) {
	case SCENARIO_EMPTY:
		ok = true; /* Já está vazio */
		break;
	case SCENARIO_SOLAR_SYSTEM:
		ok = load_solar_system(app);
		break;
	case SCENARIO_EARTH_SUN:
		ok = load_earth_sun(app);
		break;
	case SCENARIO_KERR_BLACKHOLE:
		ok = load_kerr_blackhole(app);
		break;
	case SCENARIO_BINARY_STAR:
		ok = load_binary_star(app);
		break;
	case SCENARIO_DEBUG:
		ok = load_debug(app);
		break;
	default:
		BHS_LOG_WARN("Cenário desconhecido: %d", type);
		return false;
	}

	if (ok) {
		/* Mapear para enum do app_state */
		switch (type) {
		case SCENARIO_SOLAR_SYSTEM:
			app->scenario = APP_SCENARIO_SOLAR_SYSTEM;
			break;
		case SCENARIO_EARTH_SUN:
			/* Reusando SOLAR_SYSTEM para app state por enquanto, ou NONE */
			app->scenario = APP_SCENARIO_SOLAR_SYSTEM; 
			break;
		case SCENARIO_KERR_BLACKHOLE:
			app->scenario = APP_SCENARIO_KERR_BLACKHOLE;
			break;
		case SCENARIO_BINARY_STAR:
			app->scenario = APP_SCENARIO_BINARY_STAR;
			break;
		case SCENARIO_DEBUG:
			app->scenario = APP_SCENARIO_DEBUG;
			break;
		default:
			app->scenario = APP_SCENARIO_NONE;
			break;
		}
		set_camera_for_scenario(app, type);
		app->accumulated_time = 0.0;
		BHS_LOG_INFO("Cenário '%s' carregado com sucesso",
			     scenario_get_name(type));
	}

	return ok;
}

void scenario_unload(struct app_state *app)
{
	if (!app || !app->scene)
		return;

	/*
	 * TODO: Implementar bhs_scene_clear() na engine
	 * Por enquanto, removemos corpo por corpo (ineficiente mas funciona)
	 */
	int count;
	while ((void)bhs_scene_get_bodies(app->scene, &count), count > 0) {
		bhs_scene_remove_body(app->scene, 0);
	}

	/* Reseta contadores de nomes para próximo cenário começar do 1 */
	bhs_scene_reset_counters();

	app->scenario = APP_SCENARIO_NONE;
}

const char *scenario_get_name(enum scenario_type type)
{
	switch (type) {
	case SCENARIO_EMPTY:
		return "Espaço Vazio";
	case SCENARIO_SOLAR_SYSTEM:
		return "Sistema Solar";
	case SCENARIO_EARTH_SUN:
		return "Terra e Sol";
	case SCENARIO_KERR_BLACKHOLE:
		return "Black Hole Kerr";
	case SCENARIO_BINARY_STAR:
		return "Sistema Binário";
	case SCENARIO_DEBUG:
		return "Debug";
	default:
		return "Desconhecido";
	}
}
