/**
 * @file world.c
 * @brief Gerenciamento do Mundo ECS
 */

#include "lib/ecs/ecs.h"

static bhs_world_handle g_world = NULL;

void bhs_world_init()
{
	g_world = bhs_ecs_create_world();
}

bhs_world_handle bhs_world_get()
{
	return g_world;
}
