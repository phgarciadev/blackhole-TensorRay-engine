/**
 * @file test_rhi_boot.c
 * @brief Teste de Inicialização do RHI (Vulkan)
 *
 * Verifica:
 * - Criação do device Vulkan
 * - Criação de buffers
 * - Criação de texturas
 * - Shutdown sem crash
 *
 * Este teste requer drivers Vulkan instalados.
 * Se falhar no CI/CD sem GPU, pule com --skip-rhi.
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
 * test_gpu_device_creation - Testa criação do device
 */
static void test_gpu_device_creation(bhs_platform_t platform, bhs_window_t window)
{
	BHS_TEST_SECTION("GPU Device Creation");

	struct bhs_gpu_device_config cfg = {
		.window = window,
		.enable_validation = true,
		.enable_debug = true,
	};

	bhs_gpu_device_t device = bhs_gpu_device_create(platform, &cfg);
	BHS_TEST_ASSERT_NOT_NULL(device, "bhs_gpu_device_create() retornou válido");

	if (device) {
		bhs_gpu_device_destroy(device);
		BHS_TEST_ASSERT(1, "bhs_gpu_device_destroy() sem crash");
	}
}

/**
 * test_buffer_creation - Testa criação de buffers GPU
 */
static void test_buffer_creation(bhs_platform_t platform, bhs_window_t window)
{
	BHS_TEST_SECTION("GPU Buffer Creation");

	struct bhs_gpu_device_config dev_cfg = {
		.window = window,
		.enable_validation = true,
	};
	bhs_gpu_device_t device = bhs_gpu_device_create(platform, &dev_cfg);

	if (!device) {
		BHS_TEST_ASSERT(0, "Device não criado, pulando teste de buffer");
		return;
	}

	/* Cria buffer de vértices */
	struct bhs_gpu_buffer_config buf_cfg = {
		.size = 1024,
		.usage = BHS_BUFFER_VERTEX,
		.memory = BHS_MEMORY_CPU_VISIBLE,
		.label = "Test Vertex Buffer",
	};

	bhs_gpu_buffer_t buffer = NULL;
	int result = bhs_gpu_buffer_create(device, &buf_cfg, &buffer);
	
	BHS_TEST_ASSERT_EQ(result, BHS_GPU_OK, "bhs_gpu_buffer_create() retornou OK");
	BHS_TEST_ASSERT_NOT_NULL(buffer, "Buffer criado não é NULL");

	if (buffer) {
		/* Testa mapeamento */
		void *mapped = bhs_gpu_buffer_map(buffer);
		BHS_TEST_ASSERT_NOT_NULL(mapped, "bhs_gpu_buffer_map() retornou válido");

		if (mapped) {
			/* Escreve dados de teste */
			memset(mapped, 0xAB, 1024);
			bhs_gpu_buffer_unmap(buffer);
			BHS_TEST_ASSERT(1, "Escrita e unmap sem crash");
		}

		bhs_gpu_buffer_destroy(buffer);
	}

	bhs_gpu_device_destroy(device);
}

/**
 * test_swapchain_creation - Testa criação de swapchain
 */
static void test_swapchain_creation(bhs_platform_t platform, bhs_window_t window)
{
	BHS_TEST_SECTION("Swapchain Creation");

	struct bhs_gpu_device_config dev_cfg = {
		.window = window,
		.enable_validation = true,
	};
	bhs_gpu_device_t device = bhs_gpu_device_create(platform, &dev_cfg);

	if (!device) {
		BHS_TEST_ASSERT(0, "Device não criado, pulando teste de swapchain");
		return;
	}

	struct bhs_gpu_swapchain_config swap_cfg = {
		.width = 800,
		.height = 600,
		.vsync = true,
		.triple_buffer = false,
	};

	bhs_gpu_swapchain_t swapchain = NULL;
	int result = bhs_gpu_swapchain_create(device, &swap_cfg, &swapchain);

	BHS_TEST_ASSERT_EQ(result, BHS_GPU_OK, "Swapchain criado com sucesso");
	BHS_TEST_ASSERT_NOT_NULL(swapchain, "Swapchain não é NULL");

	if (swapchain) {
		bhs_gpu_swapchain_destroy(swapchain);
		BHS_TEST_ASSERT(1, "Swapchain destruído sem crash");
	}

	bhs_gpu_device_destroy(device);
}

/* ============================================================================
 * MAIN
 * ============================================================================
 */

int main(void)
{
	bhs_log_init();
	bhs_log_set_level(BHS_LOG_LEVEL_WARN);

	BHS_TEST_BEGIN("RHI Boot Tests (Vulkan)");

	/* Setup comum */
	bhs_platform_t platform = bhs_platform_create();
	if (!platform) {
		printf("  [SKIP] Platform não disponível\n");
		BHS_TEST_END();
	}

	struct bhs_window_config win_cfg = {
		.title = "RHI Test",
		.width = 800,
		.height = 600,
	};
	bhs_window_t window = bhs_window_create(platform, &win_cfg);
	if (!window) {
		printf("  [SKIP] Janela não disponível\n");
		bhs_platform_destroy(platform);
		BHS_TEST_END();
	}

	/* Testes */
	test_gpu_device_creation(platform, window);
	test_buffer_creation(platform, window);
	test_swapchain_creation(platform, window);

	/* Cleanup */
	bhs_window_destroy(window);
	bhs_platform_destroy(platform);
	bhs_log_shutdown();

	BHS_TEST_END();
}
