/**
 * @file lib.h
 * @brief Engine Library - Física de alto nível
 *
 * "Onde a teoria vira prática. Ou tela preta."
 *
 * Este header agrupa os motores de simulação:
 * - Geodésicas (trajetórias de luz e matéria)
 * - Disco de acreção (cores, temperatura, Doppler)
 *
 * Depende de: lib/core.h
 *
 * Uso típico:
 *   #include "engine/engine.h"
 *
 *   struct bhs_geodesic ray;
 *   bhs_geodesic_propagate(&ray, &bh, &config);
 */

#ifndef BHS_ENGINE_LIB_H
#define BHS_ENGINE_LIB_H

/* Dependência: Core */
#include "math/core.h"

/* ============================================================================
 * GEODÉSICAS
 * ============================================================================
 */

#include "engine/geodesic/geodesic.h"

/* ============================================================================
 * DISCO DE ACREÇÃO
 * ============================================================================
 */

#include "engine/disk/disk.h"

#endif /* BHS_ENGINE_LIB_H */
