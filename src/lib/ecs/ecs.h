/**
 * @file ecs.h
 * @brief Entity Component System - "Data over Objects"
 *
 * Arquitetura Data-Oriented leve para simulação física.
 * - Entities: IDs (uint32_t)
 * - Components: Arrays contíguos (SoA)
 * - Systems: Funções que operam em arrays
 */

#ifndef BHS_LIB_ECS_H
#define BHS_LIB_ECS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "lib/math/bhs_math.h"

/* ============================================================================
 * TIPOS BÁSICOS
 * ============================================================================
 */

typedef uint32_t bhs_entity_id;
#define BHS_ENTITY_INVALID 0
#define BHS_MAX_ENTITIES 10000

/* 
 * Handle opaco para o Mundo ECS.
 * Contém todos os arrays de componentes e gerenciamento de IDs.
 */
typedef struct bhs_world_t *bhs_world_handle;

/* ============================================================================
 * API DO MUNDO (ENTITY MANAGER)
 * ============================================================================
 */

bhs_world_handle bhs_ecs_create_world(void);
void bhs_ecs_destroy_world(bhs_world_handle world);

/* Cria uma nova entidade vazia */
bhs_entity_id bhs_ecs_create_entity(bhs_world_handle world);

/* Destrói entidade e recicla ID */
void bhs_ecs_destroy_entity(bhs_world_handle world, bhs_entity_id entity);

/* ============================================================================
 * COMPONENT REGISTRY (MACRO MAGIC SIMPLIFIED)
 * ============================================================================
 * 
 * Para não usar RTTI ou HashMaps complexos em C, vamos usar IDs estáticos
 * para tipos de componentes. O usuário define os IDs.
 */

typedef uint32_t bhs_component_type;

/* Interface genérica para adicionar/remover componentes */
void *bhs_ecs_add_component(bhs_world_handle world, bhs_entity_id entity,
			    bhs_component_type type, size_t size,
			    const void *data);
void bhs_ecs_remove_component(bhs_world_handle world, bhs_entity_id entity,
			      bhs_component_type type);
void *bhs_ecs_get_component(bhs_world_handle world, bhs_entity_id entity,
			    bhs_component_type type);

/* 
 * Iterator para Sistemas
 * Itera sobre todas as entidades que possuem a máscara de componentes especificada.
 * (Simplificação: Itera sobre tudo e checa bitmask, ou usa listas esparsas)
 */
typedef struct {
	bhs_world_handle world;
	uint32_t current_idx;
	// ... filtering state
} bhs_ecs_query_t;

#endif /* BHS_LIB_ECS_H */
