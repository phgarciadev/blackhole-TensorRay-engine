/**
 * @file components.h
 * @brief Componentes Padrão da Engine
 *
 * "Dados não têm comportamento. Comportamento não tem estado."
 * - O Mantra ECS que você deveria tatuar na testa
 *
 * Cada componente é um POD (Plain Old Data). Sem métodos, sem lógica.
 * Sistemas operam sobre componentes. Componentes são só dados.
 */

#ifndef BHS_ENGINE_COMPONENTS_H
#define BHS_ENGINE_COMPONENTS_H

#include "lib/ecs/ecs.h"
#include "lib/math/bhs_math.h"
#include "lib/math/vec4.h"

/* ============================================================================
 * IDs ESTÁTICOS DOS COMPONENTES
 * ============================================================================
 *
 * IMPORTANTE: Esses IDs são usados como bits em uma máscara.
 * Não pule números e mantenha < 32 (limite do bitmask).
 */
#define BHS_COMP_TRANSFORM	1
#define BHS_COMP_PHYSICS	2
#define BHS_COMP_COLLIDER	3
#define BHS_COMP_CELESTIAL	4
#define BHS_COMP_KERR_METRIC	5

/* Máscara helper pra queries comuns */
#define BHS_MASK_MOVABLE	((1 << BHS_COMP_TRANSFORM) | (1 << BHS_COMP_PHYSICS))
#define BHS_MASK_COLLIDABLE	(BHS_MASK_MOVABLE | (1 << BHS_COMP_COLLIDER))

/* ============================================================================
 * TRANSFORM COMPONENT
 * ============================================================================
 * Posição e Orientação no espaço.
 * Alinhado pra upload direto em GPU buffers.
 */
typedef struct {
	BHS_ALIGN(16)
	struct bhs_vec4 position;	/* xyz = posição, w = scale uniforme */
	struct bhs_vec4 rotation;	/* Quaternion (x, y, z, w) */
} bhs_transform_component;

/* ============================================================================
 * PHYSICS COMPONENT
 * ============================================================================
 * Velocidade, Massa, Forças acumuladas.
 * Integrador atualiza posição baseado nesses dados.
 */
typedef struct {
	BHS_ALIGN(16)
	struct bhs_vec4 velocity;	/* xyz = velocidade linear, w = padding */
	struct bhs_vec4 force_accumulator; /* Forças acumuladas pro frame */
	real_t mass;
	real_t inverse_mass;		/* Cache: 1/mass (0 se massa infinita) */
	real_t drag;			/* Coeficiente de arrasto (0-1) */
	real_t restitution;		/* Coeficiente de restituição (bounce) */
} bhs_physics_component;

/* ============================================================================
 * COLLIDER COMPONENT
 * ============================================================================
 * Forma de colisão. Separado da física porque:
 * - Nem todo objeto físico colide (partículas de efeito)
 * - Triggers detectam sem aplicar física
 */
enum bhs_collider_type {
	BHS_COLLIDER_SPHERE,
	BHS_COLLIDER_BOX,
	BHS_COLLIDER_CAPSULE,
	/* Adicione outros conforme necessário */
};

typedef struct {
	enum bhs_collider_type type;
	union {
		real_t sphere_radius;
		struct bhs_vec3 box_half_extents;
		struct {
			real_t capsule_radius;
			real_t capsule_height;
		};
	};
	bool is_trigger;	/* Se true: detecta colisão, mas não resolve */
	uint32_t layer;		/* Bitmask de camadas (quem colide com quem) */
} bhs_collider_component;

/* ============================================================================
 * CELESTIAL COMPONENT
 * ============================================================================
 * Dados astrofísicos. O que faz uma entidade ser "especial":
 * estrela, planeta, buraco negro, etc.
 *
 * Separado da física porque o motor físico não precisa saber
 * se algo é uma estrela pra calcular gravidade.
 */
enum bhs_celestial_type {
	BHS_CELESTIAL_PLANET,
	BHS_CELESTIAL_STAR,
	BHS_CELESTIAL_BLACKHOLE,
	BHS_CELESTIAL_ASTEROID,
	BHS_CELESTIAL_MOON,
};

enum bhs_star_class {
	BHS_STAR_MAIN_SEQUENCE,
	BHS_STAR_RED_GIANT,
	BHS_STAR_WHITE_DWARF,
	BHS_STAR_NEUTRON,
	/* Adicione outros conforme necessário */
};

typedef struct {
	enum bhs_celestial_type type;
	real_t temperature;		/* Kelvin */
	real_t luminosity;		/* Watts (ou em L☉) */
	real_t radius_physical;		/* Raio físico real (não o de colisão) */
	real_t age;			/* Idade em anos */

	/* Dados específicos (union pra economizar memória) */
	union {
		struct {
			real_t albedo;		/* Refletividade (planetas) */
			real_t surface_pressure; /* Pressão atmosférica (Pa) */
		} planet;
		struct {
			enum bhs_star_class star_class;
			real_t metallicity;	/* Fração Z */
		} star;
	};
} bhs_celestial_component;

/* ============================================================================
 * KERR METRIC COMPONENT
 * ============================================================================
 * Parâmetros relativísticos para buracos negros rotativos.
 * Separado porque nem todo buraco negro precisa (Schwarzschild = spin 0).
 */
typedef struct {
	real_t spin_parameter;		/* a/M: 0 = Schwarzschild, 1 = extremo */
	real_t event_horizon_r;		/* Raio do horizonte de eventos (cache) */
	real_t ergosphere_r;		/* Raio da ergosfera no equador (cache) */
	real_t isco_r;			/* Innermost Stable Circular Orbit (cache) */
} bhs_kerr_metric_component;

/* ============================================================================
 * HELPER: Criação de componentes
 * ============================================================================
 */

/**
 * Cria componente de física com valores padrão sensatos.
 */
static inline bhs_physics_component bhs_physics_make(real_t mass, real_t drag)
{
	return (bhs_physics_component){
		.velocity = bhs_vec4_zero(),
		.force_accumulator = bhs_vec4_zero(),
		.mass = mass,
		.inverse_mass = (mass > 0) ? (1.0 / mass) : 0.0,
		.drag = drag,
		.restitution = 0.5,
	};
}

/**
 * Cria componente de colisor esférico.
 */
static inline bhs_collider_component bhs_collider_sphere(real_t radius,
							 bool is_trigger)
{
	return (bhs_collider_component){
		.type = BHS_COLLIDER_SPHERE,
		.sphere_radius = radius,
		.is_trigger = is_trigger,
		.layer = 0xFFFFFFFF, /* Colide com tudo por padrão */
	};
}

#endif /* BHS_ENGINE_COMPONENTS_H */

