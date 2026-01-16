/**
 * @file main.c
 * @brief Ponto de Entrada - O Nirvana do Minimalismo
 *
 * "30 linhas de código que movem o universo."
 *
 * Este arquivo é propositalmente minimalista. Toda a lógica real
 * está em app_state.c, scenario_mgr.c e input_layer.c.
 *
 * Se você está editando este arquivo, provavelmente está fazendo errado.
 * A não ser que esteja mudando o cenário inicial. Aí pode.
 */

#include "app_state.h"
#include "simulation/scenario_mgr.h"
#include "gui-framework/log.h"

#include <stdlib.h>

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	/* 1. Boot Application */
	struct app_state app = {0};
	if (!app_init(&app, "BlackHole TensorRay", 1920, 1080)) {
		BHS_LOG_FATAL("Falha ao inicializar aplicação");
		return EXIT_FAILURE;
	}

	/* 2. Load Initial Scenario */
	const char *scene_env = getenv("BHS_DEBUG_SCENE");
	enum scenario_type scenario = SCENARIO_EMPTY;

	if (scene_env) {
		switch (scene_env[0]) {
		case '0':
			scenario = SCENARIO_EMPTY;
			break;
		case '1':
			scenario = SCENARIO_DEBUG;
			break;
		case '2':
			scenario = SCENARIO_SOLAR_SYSTEM;
			break;
		case '3':
			scenario = SCENARIO_KERR_BLACKHOLE;
			break;
		case '4':
			scenario = SCENARIO_BINARY_STAR;
			break;
		}
	}

	scenario_load(&app, scenario);

	/* 3. Run Loop */
	app_run(&app);

	/* 4. Shutdown */
	app_shutdown(&app);

	return EXIT_SUCCESS;
}
