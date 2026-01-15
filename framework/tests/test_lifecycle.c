/**
 * @file test_lifecycle.c
 * @brief Teste de Ciclo de Vida do Framework
 *
 * Verifica:
 * - Inicialização e shutdown do framework
 * - Criação e destruição de janelas
 * - Ausência de vazamento de memória (usar valgrind)
 *
 * Executar com:
 *   valgrind --leak-check=full ./build/framework/tests/test_lifecycle
 */

#include "test_runner.h"
#include "framework/log.h"
#include "framework/platform/platform.h"
#include "framework/rhi/renderer.h"

/* ============================================================================
 * TESTES
 * ============================================================================
 */

/**
 * test_platform_init - Testa inicialização da plataforma
 */
static void test_platform_init(void)
{
	BHS_TEST_SECTION("Platform Init/Shutdown");

	/* Inicializa */
	bhs_platform_t platform = NULL;
	int res = bhs_platform_init(&platform);
	BHS_TEST_ASSERT_EQ(res, BHS_PLATFORM_OK, "bhs_platform_init() retornou OK");
	BHS_TEST_ASSERT_NOT_NULL(platform, "Platform handle válido");

	/* Shutdown */
	if (platform) {
		bhs_platform_shutdown(platform);
		BHS_TEST_ASSERT(1, "bhs_platform_shutdown() executou sem crash");
	}
}

/**
 * test_window_lifecycle - Testa criação/destruição de janela
 */
static void test_window_lifecycle(void)
{
	BHS_TEST_SECTION("Window Lifecycle");

	/* Setup */
	bhs_platform_t platform = NULL;
	bhs_platform_init(&platform);
	BHS_TEST_ASSERT_NOT_NULL(platform, "Platform criada");

	/* Cria janela */
	struct bhs_window_config cfg = {
		.title = "Test Window",
		.width = 800,
		.height = 600,
		.x = BHS_WINDOW_POS_CENTERED,
		.y = BHS_WINDOW_POS_CENTERED,
		.flags = BHS_WINDOW_RESIZABLE,
	};

	bhs_window_t window = NULL;
	int res = bhs_window_create(platform, &cfg, &window);
	
	BHS_TEST_ASSERT_EQ(res, BHS_PLATFORM_OK, "bhs_window_create() retornou OK");
	BHS_TEST_ASSERT_NOT_NULL(window, "Window handle válido");

	/* Verifica dimensões */
	if (window) {
		int w, h;
		bhs_window_get_size(window, &w, &h);
		BHS_TEST_ASSERT_EQ(w, 800, "Largura da janela = 800");
		BHS_TEST_ASSERT_EQ(h, 600, "Altura da janela = 600");

		/* Destrói */
		bhs_window_destroy(window);
		BHS_TEST_ASSERT(1, "bhs_window_destroy() executou sem crash");
	}

	bhs_platform_shutdown(platform);
}

/**
 * test_multiple_cycles - Testa ciclos repetidos
 */
static void test_multiple_cycles(void)
{
	BHS_TEST_SECTION("Multiple Init/Shutdown Cycles");

	for (int i = 0; i < 5; i++) {
		bhs_platform_t platform = NULL;
		bhs_platform_init(&platform);
		BHS_TEST_ASSERT_NOT_NULL(platform, "Ciclo: platform criada");

		struct bhs_window_config cfg = {
			.title = "Cycle Test",
			.width = 320,
			.height = 240,
			.flags = 0,
		};

		bhs_window_t win = NULL;
		bhs_window_create(platform, &cfg, &win);
		BHS_TEST_ASSERT_NOT_NULL(win, "Ciclo: janela criada");

		bhs_window_destroy(win);
		bhs_platform_shutdown(platform);
	}

	BHS_TEST_ASSERT(1, "5 ciclos completos sem leak/crash");
}

/* ============================================================================
 * MAIN
 * ============================================================================
 */

int main(void)
{
	bhs_log_init();
	bhs_log_set_level(BHS_LOG_LEVEL_WARN); /* Menos spam durante testes */

	BHS_TEST_BEGIN("Framework Lifecycle Tests");

	test_platform_init();
	test_window_lifecycle();
	test_multiple_cycles();

	bhs_log_shutdown();
	BHS_TEST_END();
}
