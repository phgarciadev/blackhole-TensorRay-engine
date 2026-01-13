/**
 * @file components.h
 * @brief Componentes Padrão da Engine
 */

#ifndef BHS_ENGINE_COMPONENTS_H
#define BHS_ENGINE_COMPONENTS_H

#include "lib/ecs/ecs.h"
#include "lib/math/bhs_math.h"
#include "lib/math/vec4.h"

/* IDs Estáticos dos Componentes */
#define BHS_COMP_TRANSFORM 1
#define BHS_COMP_PHYSICS 2

/* 
 * Transform Component
 * Posição e Orientação.
 * Alinhado para upload direto em GPU buffers se necessário.
 */
typedef struct {
	BHS_ALIGN(16)
	struct bhs_vec4 position; // w = scale (uniform) ou padding
	struct bhs_vec4 rotation; // Quaternion (xyzw)
} bhs_transform_component;

/*
 * Physics Component
 * Velocidade, Massa, Forças.
 */
typedef struct {
	BHS_ALIGN(16) struct bhs_vec4 velocity;
	struct bhs_vec4 force_accumulator;
	real_t mass;
	real_t inverse_mass;
	real_t drag; // Coeficiente de arrasto
	real_t padding;
} bhs_physics_component;

#endif /* BHS_ENGINE_COMPONENTS_H */
