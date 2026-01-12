/**
 * @file scene.h
 * @brief Orquestrador da Simulação
 *
 * "O Diretor do show. Diz quem entra, quem sai e quem colide."
 *
 * Responsável por:
 * - Gerenciar ciclo de vida de todos os corpos
 * - Gerenciar a malha do espaço-tempo
 * - Executar o loop de física (integração)
 */

#ifndef BHS_SIM_SCENE_H
#define BHS_SIM_SCENE_H

#include "engine/body/body.h"
#include "engine/engine.h"
#include "engine/spacetime/spacetime.h" // Incluindo explicitamente

/**
 * Handle para a cena
 */
typedef struct bhs_scene_impl *bhs_scene_t;

/* ============================================================================
 * API
 * ============================================================================
 */

/**
 * bhs_scene_create - Cria uma nova cena vazia
 */
bhs_scene_t bhs_scene_create(void);

/**
 * bhs_scene_destroy - Destrói a cena e TUDO que ela contém
 *
 * Libera memória de todos os corpos e da malha.
 */
void bhs_scene_destroy(bhs_scene_t scene);

/**
 * bhs_scene_init_default - Popula a cena com configuração padrão
 *
 * Cria um buraco negro no centro, uma estrela orbitando
 * e inicializa a malha do espaço-tempo.
 */
void bhs_scene_init_default(bhs_scene_t scene);

/**
 * bhs_scene_update - Avança a simulação em dt
 */
void bhs_scene_update(bhs_scene_t scene, double dt);

/**
 * bhs_scene_get_spacetime - Acesso à malha para renderização
 */
bhs_spacetime_t bhs_scene_get_spacetime(bhs_scene_t scene);

/**
 * bhs_scene_get_bodies - Acesso aos corpos para renderização
 * @count: [out] número de corpos
 */
const struct bhs_body *bhs_scene_get_bodies(bhs_scene_t scene, int *count);

#endif /* BHS_SIM_SCENE_H */
