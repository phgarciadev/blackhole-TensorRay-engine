/**
 * @file spacetime.h
 * @brief Grid do Espaço-Tempo
 *
 * "Tecido da realidade. Favor não rasgar."
 *
 * Representa a malha visual que mostra a curvatura.
 * É puramente visual/estética, baseada na métrica calculada pelo Core.
 */

#ifndef BHS_ENGINE_SPACETIME_H
#define BHS_ENGINE_SPACETIME_H

#include "lib/core.h"
#include <stdbool.h>

/* ============================================================================
 * TIPOS
 * ============================================================================
 */

/**
 * struct bhs_spacetime - A Malha
 *
 * Mantém um grid de pontos que são deformados pela gravidade.
 * Renderizada como linhas (GL_LINES).
 */
typedef struct bhs_spacetime_impl *bhs_spacetime_t;

/* ============================================================================
 * API
 * ============================================================================
 */

/**
 * bhs_spacetime_create - Cria a malha
 * @size: Tamanho físico do grid (ex: 50.0 unidades)
 * @divisions: Número de divisões (ex: 100 linhas)
 *
 * Retorna NULL em falha de memória.
 */
bhs_spacetime_t bhs_spacetime_create(double size, int divisions);

/**
 * bhs_spacetime_destroy - Libera a malha
 */
void bhs_spacetime_destroy(bhs_spacetime_t st);

/**
 * bhs_spacetime_update - Atualiza deformação baseado nos corpos
 * @st: Mão na malha
 * @bodies: Array de corpos
 * @n_bodies: Quantidade de corpos
 *
 * Calcula a métrica em cada vértice da malha e atualiza posições.
 * Deve ser chamado todo frame (ou menos, se quiser performance).
 */
void bhs_spacetime_update(bhs_spacetime_t st,
                          const void *bodies, /* struct bhs_body* */
                          int n_bodies);

/**
 * bhs_spacetime_get_render_data - Obtém dados para desenhar
 *
 * Retorna arrays de vértices para o renderer (Vulkan/Metal agnóstico).
 * O renderer chama isso pra copiar pro Vertex Buffer.
 */
void bhs_spacetime_get_render_data(bhs_spacetime_t st,
                                   float **vertices, /* Out: x,y,z, r,g,b */
                                   int *count);

/**
 * bhs_spacetime_get_divisions - Retorna número de divisões do grid
 */
int bhs_spacetime_get_divisions(bhs_spacetime_t st);

#endif /* BHS_ENGINE_SPACETIME_H */
