/**
 * @file ecs.c
 * @brief Implementação minimalista do ECS
 */

#include "lib/ecs/ecs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_COMPONENT_TYPES 32

/* 
 * Armazenamento de Componentes (Generic Pool)
 * Por simplicidade nesta fase, usamos array denso indexado por EntityID.
 * Otimização futura: Sparse Set para compactação.
 */
struct bhs_component_pool {
	size_t element_size;
	void *data;   // data[entity_id * element_size]
	bool *active; // active[entity_id]
};

struct bhs_world_t {
	uint32_t next_entity_id;
	struct bhs_component_pool components[MAX_COMPONENT_TYPES];
	// Recycled IDs queue could go here
};

bhs_world_handle bhs_ecs_create_world(void)
{
	bhs_world_handle w = calloc(1, sizeof(struct bhs_world_t));
	w->next_entity_id = 1; // 0 is Invalid
	return w;
}

void bhs_ecs_destroy_world(bhs_world_handle world)
{
	if (!world)
		return;
	for (int i = 0; i < MAX_COMPONENT_TYPES; i++) {
		if (world->components[i].data)
			free(world->components[i].data);
		if (world->components[i].active)
			free(world->components[i].active);
	}
	free(world);
}

bhs_entity_id bhs_ecs_create_entity(bhs_world_handle world)
{
	if (world->next_entity_id >= BHS_MAX_ENTITIES) {
		fprintf(stderr, "[ECS] Max entities reached!\n");
		return BHS_ENTITY_INVALID;
	}
	return world->next_entity_id++;
}

void bhs_ecs_destroy_entity(bhs_world_handle world, bhs_entity_id entity)
{
	// Mark as inactive in all pools
	for (int i = 0; i < MAX_COMPONENT_TYPES; i++) {
		if (world->components[i].active) {
			world->components[i].active[entity] = false;
		}
	}
}

/* ============================================================================
 * COMPONENT LOGIC
 * ============================================================================
 */

static void ensure_pool(bhs_world_handle world, bhs_component_type type,
			size_t size)
{
	if (type >= MAX_COMPONENT_TYPES)
		return;

	if (world->components[type].data == NULL) {
		world->components[type].element_size = size;
		world->components[type].data = calloc(BHS_MAX_ENTITIES, size);
		world->components[type].active =
			calloc(BHS_MAX_ENTITIES, sizeof(bool));
	}
}

void *bhs_ecs_add_component(bhs_world_handle world, bhs_entity_id entity,
			    bhs_component_type type, size_t size,
			    const void *data)
{
	if (entity == BHS_ENTITY_INVALID || entity >= BHS_MAX_ENTITIES)
		return NULL;

	ensure_pool(world, type, size);

	struct bhs_component_pool *pool = &world->components[type];

	// Check consistency (optional DEBUG)
	if (pool->element_size != size) {
		fprintf(stderr, "[ECS] Component size mismatch for type %d\n",
			type);
		return NULL;
	}

	size_t offset = entity * size;
	void *dest = (char *)pool->data + offset;

	if (data) {
		memcpy(dest, data, size);
	} else {
		memset(dest, 0, size);
	}

	pool->active[entity] = true;
	return dest;
}

void bhs_ecs_remove_component(bhs_world_handle world, bhs_entity_id entity,
			      bhs_component_type type)
{
	if (type >= MAX_COMPONENT_TYPES)
		return;
	if (world->components[type].active) {
		world->components[type].active[entity] = false;
	}
}

void *bhs_ecs_get_component(bhs_world_handle world, bhs_entity_id entity,
			    bhs_component_type type)
{
	if (type >= MAX_COMPONENT_TYPES)
		return NULL;
	struct bhs_component_pool *pool = &world->components[type];

	if (!pool->data || !pool->active[entity])
		return NULL;

	return (char *)pool->data + (entity * pool->element_size);
}

/* ============================================================================
 * QUERY SYSTEM
 * ============================================================================
 */

/**
 * Verifica se entidade possui todos os componentes da máscara.
 * Função interna usada pelas queries.
 */
static bool entity_matches_mask(bhs_world_handle world, bhs_entity_id entity,
				bhs_component_mask mask)
{
	for (uint32_t type = 0; type < MAX_COMPONENT_TYPES; type++) {
		if (mask & (1u << type)) {
			struct bhs_component_pool *pool = &world->components[type];
			if (!pool->active || !pool->active[entity])
				return false;
		}
	}
	return true;
}

bool bhs_ecs_entity_has_components(bhs_world_handle world, bhs_entity_id entity,
				   bhs_component_mask mask)
{
	if (!world || entity == BHS_ENTITY_INVALID || entity >= BHS_MAX_ENTITIES)
		return false;
	return entity_matches_mask(world, entity, mask);
}

void bhs_ecs_query_init(bhs_ecs_query *q, bhs_world_handle world,
			bhs_component_mask required)
{
	if (!q)
		return;

	q->world = world;
	q->required = required;
	q->current_idx = 0;
	q->count = 0;
	q->cache = NULL;
	q->use_cache = false;
}

void bhs_ecs_query_init_cached(bhs_ecs_query *q, bhs_world_handle world,
			       bhs_component_mask required)
{
	if (!q || !world)
		return;

	q->world = world;
	q->required = required;
	q->current_idx = 0;
	q->use_cache = true;

	/* Primeira passada: contar entidades matching */
	q->count = 0;
	for (bhs_entity_id id = 1; id < world->next_entity_id; id++) {
		if (entity_matches_mask(world, id, required))
			q->count++;
	}

	if (q->count == 0) {
		q->cache = NULL;
		return;
	}

	/* Aloca e preenche cache */
	q->cache = malloc(q->count * sizeof(bhs_entity_id));
	if (!q->cache) {
		fprintf(stderr, "[ECS] Falha ao alocar cache de query\n");
		q->count = 0;
		return;
	}

	uint32_t idx = 0;
	for (bhs_entity_id id = 1; id < world->next_entity_id; id++) {
		if (entity_matches_mask(world, id, required)) {
			q->cache[idx++] = id;
		}
	}
}

bool bhs_ecs_query_next(bhs_ecs_query *q, bhs_entity_id *out_entity)
{
	if (!q || !q->world || !out_entity)
		return false;

	if (q->use_cache) {
		/* Modo cache: itera sobre array pré-computado */
		if (q->current_idx >= q->count)
			return false;
		*out_entity = q->cache[q->current_idx++];
		return true;
	}

	/* Modo on-the-fly: itera e filtra */
	while (q->current_idx < q->world->next_entity_id) {
		bhs_entity_id id = q->current_idx++;
		if (id == BHS_ENTITY_INVALID)
			continue;
		if (entity_matches_mask(q->world, id, q->required)) {
			*out_entity = id;
			return true;
		}
	}

	return false;
}

void bhs_ecs_query_reset(bhs_ecs_query *q)
{
	if (q)
		q->current_idx = 0;
}

void bhs_ecs_query_destroy(bhs_ecs_query *q)
{
	if (q && q->cache) {
		free(q->cache);
		q->cache = NULL;
	}
}

