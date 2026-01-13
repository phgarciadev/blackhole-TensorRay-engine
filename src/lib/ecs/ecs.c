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
