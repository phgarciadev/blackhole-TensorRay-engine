/**
 * @file scene.c
 * @brief Implementação do Orquestrador (ECS Adapter)
 */

#include "engine/scene/scene.h"
#include "engine/ecs/ecs.h"
#include "engine/components/components.h"
#include "engine/scene/scene.h"
#include "engine/physics/spacetime/spacetime.h" // Pending verification of path
#include "src/simulation/components/sim_components.h" // Game components

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

extern void bhs_engine_init(void);

// Temporary storage for legacy get_bodies
#define MAX_BODIES 128
static struct bhs_body g_legacy_bodies[MAX_BODIES];
static bhs_spacetime_t g_spacetime_cache = NULL;

extern bhs_world_handle bhs_engine_get_world_internal(void); // Need a way to get world if opaque

// Hack: we need access to the world handle hidden in engine.c
// Ideally engine.h would expose a getter for internal systems, or scene is part of engine.
// Let's assume for this Refactor that we can access it via a helper or scene creates it if engine isn't init?
// Actually engine_init creates it. 
// We will add `bhs_engine_get_world()` to engine.h or just rely on global?
// Let's declare it external here for "Friend" access within engine module.
// In real engine, this would be cleaner.
bhs_world_handle bhs_engine_get_world_unsafe(void) {
    // This function must be implemented in engine.c
    return NULL; // implementation required
}

struct bhs_scene_impl {
	bhs_world_handle world;
    bhs_spacetime_t spacetime; // Keep spacetime managed here for now
};

bhs_scene_t bhs_scene_create(void)
{
    // Ensure engine is up
    bhs_engine_init();

	bhs_scene_t scene = calloc(1, sizeof(struct bhs_scene_impl));
	if (!scene) return NULL;

    // We don't get the world handle easily? 
    // Let's modify engine.c to expose `bhs_engine_get_world()`?
    // Or scene IS the owner of logic? 
    // Plan: Engine Core owns world. Scene is just a high level helper.
    // For now, let's assume we can get the world or we rely on engine global state commands.
	
    /*
     * Spacetime Grid (malha visual da curvatura)
     *
     * Tamanho: 500 unidades (cobre área ao redor do Sol)
     * Com 1 AU = 50 unidades, o grid cobre ~10 AU de raio.
     *
     * Divisões: 80 → células de ~6.25u cada
     * Isso dá resolução suficiente para ver a curvatura suave.
     *
     * Para ver órbitas externas (Saturno em 9.5 AU = 475u),
     * o grid atual é adequado.
     */
    scene->spacetime = bhs_spacetime_create(500.0, 80);
    g_spacetime_cache = scene->spacetime;
    
    // Connect to Engine World
    extern bhs_world_handle bhs_engine_get_world_internal(void);
    scene->world = bhs_engine_get_world_internal();

	return scene;
}

void bhs_scene_destroy(bhs_scene_t scene)
{
	if (!scene) return;
	if (scene->spacetime) bhs_spacetime_destroy(scene->spacetime);
	free(scene);
}

void bhs_scene_init_default(bhs_scene_t scene)
{
    // Default initialization is now handled by the Application (src/)
    // Engine provides an empty scene.
    (void)scene;
}

void bhs_scene_update(bhs_scene_t scene, double dt)
{
    // Run Engine Update
    bhs_engine_update(dt);

    // Sync spacetime (visual)
    // We need to pass bodies to spacetime.
    // This is where we reconstruct the legacy body array for visualization tools
    // that haven't been ported to ECS.
    int count = 0;
    const struct bhs_body *bodies = bhs_scene_get_bodies(scene, &count);
    
    if (scene->spacetime) {
        bhs_spacetime_update(scene->spacetime, bodies, count);
    }
}

bhs_spacetime_t bhs_scene_get_spacetime(bhs_scene_t scene)
{
	return scene ? scene->spacetime : NULL;
}

bhs_world_handle bhs_scene_get_world(bhs_scene_t scene)
{
    return scene ? scene->world : NULL;
}

// LEGACY ADAPTER: Reconstruct bhs_body structs from ECS components
const struct bhs_body *bhs_scene_get_bodies(bhs_scene_t scene, int *count)
{
    if (!scene || !scene->world) {
        *count = 0; 
        return NULL; 
    }
    bhs_world_handle world = scene->world;

    bhs_ecs_query q;
    // Query movable things
    bhs_ecs_query_init(&q, world, (1<<BHS_COMP_TRANSFORM)); // Get all with transform?

    int idx = 0;
    bhs_entity_id id;
    while(bhs_ecs_query_next(&q, &id) && idx < MAX_BODIES) {
        bhs_transform_t *t = bhs_ecs_get_component(world, id, BHS_COMP_TRANSFORM);
        bhs_physics_t *p = bhs_ecs_get_component(world, id, BHS_COMP_PHYSICS);
        bhs_celestial_component *c = bhs_ecs_get_component(world, id, BHS_COMP_CELESTIAL); // Defined in src?
        // Wait, Engine doesn't know BHS_COMP_CELESTIAL unless I include it.
        // using ID cast.
        
        if (!t) continue;

        struct bhs_body *b = &g_legacy_bodies[idx];
        
        // Map Transform
        b->state.pos = t->position;
        // Map Physics
        if (p) {
            b->state.vel = p->velocity;
            b->state.mass = p->mass;
            b->is_fixed = p->is_static;
            b->is_alive = true; // ECS only returns alive
        } else {
            b->state.vel = (struct bhs_vec3){0};
            b->state.mass = 0;
        }

        // Map Celestial
        if (c) {
            strncpy(b->name, c->name, 31);
            if (c->type == BHS_CELESTIAL_PLANET) {
                b->type = BHS_BODY_PLANET;
                b->state.radius = c->data.planet.radius;
                b->color = c->data.planet.color;
            } else if (c->type == BHS_CELESTIAL_STAR) {
                b->type = BHS_BODY_STAR;
                 // Defaults/Map
            } else if (c->type == BHS_CELESTIAL_BLACKHOLE) {
                b->type = BHS_BODY_BLACKHOLE;
            }
        } else {
            // Fallback defaults
            b->type = BHS_BODY_ASTEROID;
            b->state.radius = 1.0;
            b->color = (struct bhs_vec3){1,1,1};
        }

        idx++;
    }

    *count = idx;
    return g_legacy_bodies;
}

// LEGACY ADAPTER: Create entity from struct
bool bhs_scene_add_body_struct(bhs_scene_t scene, struct bhs_body b)
{
    return bhs_scene_add_body_named(scene, b.type, b.state.pos, b.state.vel,
                                    b.state.mass, b.state.radius, b.color, b.name);
}

/* Contador global para nomes únicos (lazy, mas funciona) */
static int g_planet_counter = 0;
static int g_star_counter = 0;
static int g_asteroid_counter = 0;

void bhs_scene_reset_counters(void)
{
    g_planet_counter = 0;
    g_star_counter = 0;
    g_asteroid_counter = 0;
}

bool bhs_scene_add_body(bhs_scene_t scene, enum bhs_body_type type,
			struct bhs_vec3 pos, struct bhs_vec3 vel, double mass,
			double radius, struct bhs_vec3 color)
{
    /* Gera nome único baseado no tipo */
    char name[64];
    switch (type) {
    case BHS_BODY_PLANET:
        snprintf(name, sizeof(name), "Planet %d", ++g_planet_counter);
        break;
    case BHS_BODY_STAR:
        snprintf(name, sizeof(name), "Star %d", ++g_star_counter);
        break;
    case BHS_BODY_BLACKHOLE:
        snprintf(name, sizeof(name), "Black Hole");
        break;
    case BHS_BODY_ASTEROID:
        snprintf(name, sizeof(name), "Asteroid %d", ++g_asteroid_counter);
        break;
    default:
        snprintf(name, sizeof(name), "Body");
        break;
    }
    return bhs_scene_add_body_named(scene, type, pos, vel, mass, radius, color, name);
}

bool bhs_scene_add_body_named(bhs_scene_t scene, enum bhs_body_type type,
			      struct bhs_vec3 pos, struct bhs_vec3 vel,
			      double mass, double radius, struct bhs_vec3 color,
			      const char *name)
{
    extern bhs_world_handle bhs_engine_get_world_internal(void);
    bhs_world_handle world = bhs_engine_get_world_internal();
    if (!world) return false;
    (void)scene; // Mark unused

    bhs_entity_id e = bhs_ecs_create_entity(world);

    // Transform
    bhs_transform_t t = {
        .position = pos,
        .scale = {radius, radius, radius}, // Approximate visual scale
        .rotation = {0,0,0,1}
    };
    bhs_ecs_add_component(world, e, BHS_COMP_TRANSFORM, sizeof(t), &t);

    // Physics
    bhs_physics_t p = {
        .mass = mass,
        .velocity = vel,
        .is_static = (type == BHS_BODY_BLACKHOLE)
    };
    bhs_ecs_add_component(world, e, BHS_COMP_PHYSICS, sizeof(p), &p);

    // Celestial
    bhs_celestial_component c = {0};
    if (type == BHS_BODY_PLANET) {
        c.type = BHS_CELESTIAL_PLANET;
        c.data.planet.radius = radius;
        c.data.planet.color = color;
        strncpy(c.name, name ? name : "Planet", 63);
    } else if (type == BHS_BODY_BLACKHOLE) {
        c.type = BHS_CELESTIAL_BLACKHOLE;
        strncpy(c.name, name ? name : "Black Hole", 63);
    } else if (type == BHS_BODY_STAR) {
        c.type = BHS_CELESTIAL_STAR;
        strncpy(c.name, name ? name : "Star", 63);
    } else {
        c.type = BHS_CELESTIAL_ASTEROID;
        strncpy(c.name, name ? name : "Asteroid", 63);
    }
    // Note: BHS_COMP_CELESTIAL ID must match what is used in src/
    // Since we included sim_components.h which defines BHS_COMP_CELESTIAL, use that.
    bhs_ecs_add_component(world, e, BHS_COMP_CELESTIAL, sizeof(c), &c);

    return true;
}


void bhs_scene_remove_body(bhs_scene_t scene, int index)
{
    if (!scene || !scene->world) return;
    bhs_world_handle world = scene->world;

    bhs_ecs_query q;
    // Query movable things - Same mask as get_bodies
    bhs_ecs_query_init(&q, world, (1<<BHS_COMP_TRANSFORM)); 

    int idx = 0;
    bhs_entity_id id;
    bool found = false;

    while(bhs_ecs_query_next(&q, &id)) {
        bhs_transform_t *t = bhs_ecs_get_component(world, id, BHS_COMP_TRANSFORM);
        if (!t) continue;

        if (idx == index) {
            found = true;
            break; /* Found our victim */
        }
        idx++;
    }

    if (found) {
        printf("[SCENE] Destroying entity ID %d (Index %d)\n", id, index);
        bhs_ecs_destroy_entity(world, id);
    } else {
        printf("[SCENE] Failed to delete: Index %d not found.\n", index);
    }
}
