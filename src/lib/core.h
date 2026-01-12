/**
 * @file lib.h
 * @brief Core Library - Fundação matemática do simulador
 *
 * "Se você não entende vetores 4D, volte pro Minecraft."
 *
 * Este header agrupa toda a infraestrutura matemática:
 * - Vetores 4D (espaço-tempo)
 * - Tensores métricos
 * - Métricas de Schwarzschild e Kerr
 *
 * Uso típico:
 *   #include "lib/core.h"
 *
 *   struct bhs_kerr bh = { .M = 1.0, .a = 0.9 };
 *   double isco = bhs_kerr_isco(&bh, true);
 */

#ifndef BHS_CORE_LIB_H
#define BHS_CORE_LIB_H

/* ============================================================================
 * MATEMÁTICA FUNDAMENTAL
 * ============================================================================
 */

#include "lib/assert.h"
#include "lib/math/vec4.h"

/* ============================================================================
 * TENSORES
 * ============================================================================
 */

#include "lib/tensor/tensor.h"

/* ============================================================================
 * ESPAÇO-TEMPO
 * ============================================================================
 */

#include "lib/spacetime/kerr.h"
#include "lib/spacetime/schwarzschild.h"

#endif /* BHS_CORE_LIB_H */
