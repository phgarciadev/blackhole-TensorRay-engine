/**
 * @file presets.h
 * @brief Corpos Celestes Pré-Definidos com Dados Físicos Reais
 *
 * "Quando você precisa de um Sol, Terra ou Lua de verdade."
 *
 * Todos os valores são dados reais da NASA/IAU, normalizados para
 * a escala da simulação onde 1 unidade = 10⁷ metros.
 */

#ifndef BHS_ENGINE_PRESETS_H
#define BHS_ENGINE_PRESETS_H

#include "engine/body/body.h"
#include "engine/scene/scene.h"

/* ============================================================================
 * CONSTANTES FÍSICAS FUNDAMENTAIS
 * ============================================================================
 */

/* Sistema Internacional */
#define BHS_CONST_G          6.67430e-11   /* Constante Gravitacional (m³/(kg·s²)) */
#define BHS_CONST_C          299792458.0   /* Velocidade da Luz (m/s) */
#define BHS_CONST_AU         1.495978707e11 /* Unidade Astronômica (m) */

/* Massas de Referência */
#define BHS_MASS_SUN         1.98847e30    /* Massa Solar (kg) */
#define BHS_MASS_EARTH       5.9722e24     /* Massa Terrestre (kg) */
#define BHS_MASS_MOON        7.342e22      /* Massa Lunar (kg) */

/* Raios de Referência */
#define BHS_RADIUS_SUN       6.9634e8      /* Raio Solar (m) */
#define BHS_RADIUS_EARTH     6.371e6       /* Raio Terrestre (m) */
#define BHS_RADIUS_MOON      1.7374e6      /* Raio Lunar (m) */

/* Distâncias Orbitais */
#define BHS_ORBIT_EARTH      1.496e11      /* Distância Terra-Sol (m) */
#define BHS_ORBIT_MOON       3.844e8       /* Distância Lua-Terra (m) */

/* ============================================================================
 * ESCALA DA SIMULAÇÃO
 * ============================================================================
 *
 * Problema: Valores reais são enormes e impossíveis de visualizar.
 * Solução: Escala normalizada onde 1 unidade = 10⁷ metros (10.000 km)
 *
 * Conversões:
 *   - Sol:        raio = 69.6 u, massa = 1.99e30 kg
 *   - Terra:      raio = 0.637 u, distância do sol = 14960 u
 *   - Lua:        raio = 0.174 u, distância da terra = 38.4 u
 *
 * Para visualização prática, usamos escala comprimida:
 *   - Raios: multiplicados por fator para ficarem visíveis
 *   - Distâncias: divididas para caber na tela
 */

#define BHS_SCALE_LENGTH     1e7           /* 1 unidade = 10⁷ metros */
#define BHS_SCALE_VISUAL     100.0         /* Fator de ampliação visual para raios */

/* Valores normalizados para a simulação */
#define BHS_SIM_RADIUS_SUN   (BHS_RADIUS_SUN / BHS_SCALE_LENGTH * BHS_SCALE_VISUAL)
#define BHS_SIM_RADIUS_EARTH (BHS_RADIUS_EARTH / BHS_SCALE_LENGTH * BHS_SCALE_VISUAL)
#define BHS_SIM_RADIUS_MOON  (BHS_RADIUS_MOON / BHS_SCALE_LENGTH * BHS_SCALE_VISUAL)

/* Distâncias normalizadas (comprimidas para visualização) */
#define BHS_SIM_ORBIT_EARTH  50.0          /* Terra a 50 unidades do Sol */
#define BHS_SIM_ORBIT_MOON   8.0           /* Lua a 8 unidades da Terra */

/* ============================================================================
 * FACTORIES DE CORPOS CELESTES
 * ============================================================================
 */

/**
 * bhs_preset_sun - Cria o Sol com dados físicos reais
 * @pos: Posição inicial (geralmente origem)
 *
 * Retorna uma struct bhs_body completamente inicializada
 * com todos os dados físicos do Sol.
 */
struct bhs_body bhs_preset_sun(struct bhs_vec3 pos);

/**
 * bhs_preset_earth - Cria a Terra em órbita
 * @sun_pos: Posição do Sol (centro da órbita)
 *
 * A Terra é criada com velocidade orbital correta para
 * órbita circular estável ao redor do Sol.
 */
struct bhs_body bhs_preset_earth(struct bhs_vec3 sun_pos);

/**
 * bhs_preset_moon - Cria a Lua em órbita da Terra
 * @earth_pos: Posição da Terra
 * @earth_vel: Velocidade da Terra (para composição)
 *
 * A Lua recebe velocidade orbital em relação à Terra
 * MAIS a velocidade da Terra.
 */
struct bhs_body bhs_preset_moon(struct bhs_vec3 earth_pos,
				struct bhs_vec3 earth_vel);

/**
 * bhs_preset_solar_system - Cria sistema Sol-Terra-Lua
 * @scene: Cena onde adicionar os corpos
 *
 * Cria os 3 corpos com órbitas estáveis e dados físicos reais.
 */
void bhs_preset_solar_system(bhs_scene_t scene);

/**
 * bhs_preset_orbital_velocity - Calcula velocidade orbital
 * @central_mass: Massa do corpo central (kg)
 * @orbital_radius: Distância orbital (unidades de simulação)
 *
 * Retorna a velocidade para órbita circular: v = sqrt(G*M/r)
 * Nota: Usa G normalizado para a escala da simulação.
 */
double bhs_preset_orbital_velocity(double central_mass, double orbital_radius);

#endif /* BHS_ENGINE_PRESETS_H */
