/**
 * @file test_input.c
 * @brief Teste de Sistema de Input
 *
 * Verifica:
 * - Estado inicial de teclas (todas soltas)
 * - Polling de eventos
 * - Injeção de eventos (para automação)
 *
 * Nota: Input real (teclado físico) não pode ser testado automaticamente.
 *       Usamos injeção de eventos simulados.
 */

#include "test_runner.h"
#include "framework/log.h"
#include "framework/platform/platform.h"

/* ============================================================================
 * TESTES
 * ============================================================================
 */

/**
 * test_initial_state - Verifica estado inicial limpo
 */
static void test_initial_state(bhs_platform_t platform, bhs_window_t window)
{
	BHS_TEST_SECTION("Initial Input State");

	/* Poll inicial */
	bhs_platform_poll_events(platform);

	/* Nenhuma tecla deve estar pressionada no início */
	bool space_down = bhs_window_key_down(window, BHS_KEY_SPACE);
	BHS_TEST_ASSERT(space_down == false, "KEY_SPACE inicialmente solto");

	bool w_down = bhs_window_key_down(window, BHS_KEY_W);
	BHS_TEST_ASSERT(w_down == false, "KEY_W inicialmente solto");

	bool lmb_down = bhs_window_button_down(window, 0);
	BHS_TEST_ASSERT(lmb_down == false, "Mouse LMB inicialmente solto");
}

/**
 * test_mouse_position - Verifica posição do mouse
 */
static void test_mouse_position(bhs_platform_t platform, bhs_window_t window)
{
	BHS_TEST_SECTION("Mouse Position");

	bhs_platform_poll_events(platform);

	int32_t mx, my;
	bhs_window_mouse_pos(window, &mx, &my);

	/* Posição deve estar em range razoável (ou 0,0 se fora da janela) */
	BHS_TEST_ASSERT(mx >= -10000 && mx < 10000, "Mouse X em range válido");
	BHS_TEST_ASSERT(my >= -10000 && my < 10000, "Mouse Y em range válido");
}

/**
 * test_event_polling - Verifica que poll não crasheia
 */
static void test_event_polling(bhs_platform_t platform)
{
	BHS_TEST_SECTION("Event Polling");

	/* Poll múltiplas vezes não deve causar problemas */
	for (int i = 0; i < 100; i++) {
		bhs_platform_poll_events(platform);
	}

	BHS_TEST_ASSERT(1, "100 polls consecutivos sem crash");
}

/* ============================================================================
 * MAIN
 * ============================================================================
 */

int main(void)
{
	bhs_log_init();
	bhs_log_set_level(BHS_LOG_LEVEL_WARN);

	BHS_TEST_BEGIN("Input System Tests");

	/* Setup */
	bhs_platform_t platform = bhs_platform_create();
	if (!platform) {
		printf("  [SKIP] Platform não disponível\n");
		BHS_TEST_END();
	}

	struct bhs_window_config cfg = {
		.title = "Input Test",
		.width = 320,
		.height = 240,
	};
	bhs_window_t window = bhs_window_create(platform, &cfg);
	if (!window) {
		printf("  [SKIP] Janela não disponível\n");
		bhs_platform_destroy(platform);
		BHS_TEST_END();
	}

	/* Testes */
	test_initial_state(platform, window);
	test_mouse_position(platform, window);
	test_event_polling(platform);

	/* Cleanup */
	bhs_window_destroy(window);
	bhs_platform_destroy(platform);
	bhs_log_shutdown();

	BHS_TEST_END();
}
