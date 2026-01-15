/**
 * @file telemetry.h
 * @brief Dashboard de Debug em Terminal (Real-time Physics Monitor)
 */

#ifndef BHS_CMD_DEBUG_TELEMETRY_H
#define BHS_CMD_DEBUG_TELEMETRY_H

#include "engine/scene/scene.h"

/**
 * @brief Imprime o estado atual da simulação no terminal.
 * Usa ANSI escape codes para tentar sobrescrever ou limpar tela se desejado.
 *
 * @param scene Cena contendo os corpos
 * @param time Tempo total de simulação
 */
void bhs_telemetry_print_scene(bhs_scene_t scene, double time, bool show_grid, double phys_ms, double render_ms);

#endif /* BHS_CMD_DEBUG_TELEMETRY_H */
