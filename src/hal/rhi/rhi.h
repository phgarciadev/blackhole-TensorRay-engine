/**
 * @file rhi.h
 * @brief Render Hardware Interface (RHI) - A Verdade sobre a GPU
 *
 * "A GPU não quer saber da sua orientação a objetos.
 *  Ela quer comandos, barreiras e dados alinhados."
 *
 * Design:
 * - Explicit Command Lists (Multi-threaded recording)
 * - Render Graph based Barriers (Automatic synth) vs Manual Barriers
 * - Bindless Resources (onde possível)
 * - Pipeline State Objects (PSO) monolíticos
 */

#ifndef BHS_HAL_RHI_H
#define BHS_HAL_RHI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "lib/math/bhs_math.h"
#include "lib/math/vec4.h"

/* ============================================================================
 * HANDLES OPACOS (Tipos Fortes)
 * ============================================================================
 */

typedef struct bhs_rhi_device_t *bhs_rhi_device_handle;
typedef struct bhs_rhi_buffer_t *bhs_rhi_buffer_handle;
typedef struct bhs_rhi_texture_t *bhs_rhi_texture_handle;
typedef struct bhs_rhi_shader_t *bhs_rhi_shader_handle;
typedef struct bhs_rhi_pipeline_t *bhs_rhi_pipeline_handle;
typedef struct bhs_rhi_cmd_list_t *bhs_rhi_cmd_list_handle;
typedef struct bhs_rhi_fence_t *bhs_rhi_fence_handle;

/* ============================================================================
 * ENUMS E FLAGS
 * ============================================================================
 */

typedef enum {
	BHS_RHI_BACKEND_VULKAN,
	BHS_RHI_BACKEND_METAL,
	BHS_RHI_BACKEND_DX12,
	BHS_RHI_BACKEND_MOCK // Para testes headless
} bhs_rhi_backend_type;

typedef enum {
	BHS_RHI_SHADER_COMPUTE,
	BHS_RHI_SHADER_GRAPHICS_VERTEX,
	BHS_RHI_SHADER_GRAPHICS_FRAGMENT
} bhs_rhi_shader_stage;

typedef enum {
	BHS_RHI_FORMAT_RGBA8_UNORM,
	BHS_RHI_FORMAT_RGBA16_FLOAT,
	BHS_RHI_FORMAT_RGBA32_FLOAT,
	BHS_RHI_FORMAT_D32_FLOAT // Depth
} bhs_rhi_format;

/* ============================================================================
 * DESCRITORES DE RECURSOS
 * ============================================================================
 */

typedef struct {
	bhs_rhi_backend_type preferred_backend;
	bool enable_validation;
	bool enable_gpu_printf; // Debug shaders
} bhs_rhi_device_desc;

typedef struct {
	const void *bytecode;
	size_t bytecode_size;
	const char *entry_point;
	bhs_rhi_shader_stage stage;
} bhs_rhi_shader_desc;

typedef struct {
	bhs_rhi_shader_handle compute_shader; // Pode vir do C-Transpiler
} bhs_rhi_compute_pipeline_desc;

typedef struct {
	uint64_t size;
	bool cpu_visible; // Se true, mapeável. Se false, GPU only.
} bhs_rhi_buffer_desc;

/* ============================================================================
 * API DO DEVICE
 * ============================================================================
 */

bhs_rhi_device_handle bhs_rhi_create_device(const bhs_rhi_device_desc *desc);
void bhs_rhi_destroy_device(bhs_rhi_device_handle dev);

/* 
 * Compilação JIT de C -> SPIR-V (integração Fase 2)
 * Se o backend for Metal/DX12, isso vai converter SPIR-V -> MSL/HLSL internamente
 */
bhs_rhi_shader_handle bhs_rhi_create_shader_from_c(bhs_rhi_device_handle dev,
						   const char *source_code,
						   bhs_rhi_shader_stage stage);

bhs_rhi_shader_handle
bhs_rhi_create_shader_from_bytecode(bhs_rhi_device_handle dev,
				    const bhs_rhi_shader_desc *desc);

/* ============================================================================
 * GERENCIAMENTO DE MEMÓRIA (LINUS STYLE: Sem mallocs escondidos no frame)
 * ============================================================================
 */

bhs_rhi_buffer_handle bhs_rhi_create_buffer(bhs_rhi_device_handle dev,
					    const bhs_rhi_buffer_desc *desc);

void *bhs_rhi_map_buffer(bhs_rhi_buffer_handle buf); // Só se cpu_visible
void bhs_rhi_unmap_buffer(bhs_rhi_buffer_handle buf);

/* ============================================================================
 * COMMAND LISTS (GRAVAÇÃO)
 * ============================================================================
 */

bhs_rhi_cmd_list_handle bhs_rhi_allocate_cmd_list(bhs_rhi_device_handle dev);

void bhs_rhi_cmd_begin(bhs_rhi_cmd_list_handle cmd);
void bhs_rhi_cmd_end(bhs_rhi_cmd_list_handle cmd);

/* Compute Dispatch */
void bhs_rhi_cmd_set_pipeline_compute(bhs_rhi_cmd_list_handle cmd,
				      bhs_rhi_pipeline_handle pipeline);

void bhs_rhi_cmd_bind_buffer(bhs_rhi_cmd_list_handle cmd, uint32_t slot,
			     bhs_rhi_buffer_handle buffer);

void bhs_rhi_cmd_dispatch(bhs_rhi_cmd_list_handle cmd, uint32_t x, uint32_t y,
			  uint32_t z);

/* 
 * BARREIRAS (The "Sync" Truth)
 * Em vez de um Render Graph completo agora, vamos expor barreiras explícitas
 * mas simples. O Render Graph pode ser construído em cima disso.
 */
void bhs_rhi_cmd_barrier(bhs_rhi_cmd_list_handle cmd
			 // TODO: Adicionar flags de memória e estágios
);

/* ============================================================================
 * SUBMISSÃO
 * ============================================================================
 */

void bhs_rhi_submit(bhs_rhi_device_handle dev, bhs_rhi_cmd_list_handle cmd);

void bhs_rhi_wait_idle(bhs_rhi_device_handle dev);

#endif /* BHS_HAL_RHI_H */
