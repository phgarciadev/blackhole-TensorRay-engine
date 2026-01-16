/**
 * @file planet_registry.c
 * @brief Implementação do Registro Dinâmico de Planetas
 */

#include "planet.h"
#include <stddef.h>
#include <stdlib.h> /* malloc */

static struct bhs_planet_registry_entry *g_registry_head = NULL;

void bhs_planet_register(const char *name, bhs_planet_getter_t getter)
{
	/* Alocação simples - em kernel real seria estático ou pool */
	/* Como isso roda pre-main (ctor), malloc é seguro na maioria das libcs */
	struct bhs_planet_registry_entry *entry = 
		malloc(sizeof(struct bhs_planet_registry_entry));
	
	if (!entry) return;
	
	entry->name = name;
	entry->getter = getter;
	entry->next = g_registry_head;
	g_registry_head = entry;
}

const struct bhs_planet_registry_entry *bhs_planet_registry_get_head(void)
{
	return g_registry_head;
}
