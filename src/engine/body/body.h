/**
 * @file body.h
 * @brief Estruturas de Dados Físicas (Kernel Style)
 *
 * "Dados dominam. Se você conhece os dados, a lógica é óbvia."
 * - Torvalds (adaptado)
 */

#ifndef BHS_ENGINE_BODY_H
#define BHS_ENGINE_BODY_H

#include "lib/math/vec4.h"

/* ============================================================================
 * ENUMS & CONSTANTES
 * ============================================================================
 */

/* ============================================================================
 * ENUMS & CONSTANTES
 * ============================================================================
 */

enum bhs_body_type {
	BHS_BODY_PLANET,
	BHS_BODY_STAR,
	BHS_BODY_BLACKHOLE,
	/* Adicione outros conforme necessário (Lua, Asteroide...) */
};

enum bhs_matter_state {
	BHS_STATE_SOLID,
	BHS_STATE_LIQUID,
	BHS_STATE_GAS,
	BHS_STATE_PLASMA
};

enum bhs_shape_type {
	BHS_SHAPE_SPHERE,
	BHS_SHAPE_ELLIPSOID,
	BHS_SHAPE_IRREGULAR
};

enum bhs_star_stage {
	BHS_STAR_MAIN_SEQUENCE,
	BHS_STAR_GIANT,
	BHS_STAR_WHITE_DWARF,
	BHS_STAR_NEUTRON
};

/* ============================================================================
 * ESTRUTURAS DE ESTADO (UNIVERSAL)
 * ============================================================================
 */

/**
 * @brief Estado Físico Universal
 * Variáveis que TODO corpo físico possui.
 */
struct bhs_body_state {
	/* Cinemática Linear */
	struct bhs_vec3 pos; /* Posição (m) */
	struct bhs_vec3 vel; /* Velocidade (m/s) */
	struct bhs_vec3 acc; /* Aceleração (m/s²) - Cache Integrador */

	/* Cinemática Angular */
	struct bhs_vec3 rot_axis; /* Eixo de Rotação (Normalizado) */
	double rot_speed; /* Velocidade Angular (rad/s) */
	double moment_inertia; /* Momento de Inércia escalar (simplificado) */

	/* Invariantes de Massa/Forma */
	double mass; /* Massa (kg) */
	double radius; /* Raio Efetivo (m) */
	enum bhs_shape_type shape; /* Forma geométrica básica */
};

/* ============================================================================
 * ESTRUTURAS ESPECÍFICAS (PROPRIEDADES)
 * ============================================================================
 */

/**
 * @brief Dados exclusivos de Planetas (Rochosos/Gasosos)
 */
struct bhs_planet_data {
	/* Essencial */
	double density; /* Densidade média (kg/m³) */
	double axis_tilt; /* Obliquidade (radianos) */
	double albedo; /* Refletividade (0..1) */

	/* Atmosfera */
	bool has_atmosphere;
	double surface_pressure; /* Pressão atmosférica (Pa) */
	double atmosphere_mass; /* Massa atm (kg) */
	/* Composição simplificada para string */
	char composition[64]; /* ex: "21% O2, 78% N2" */

	/* Termodinâmico */
	double temperature; /* Temperatura média (K) */
	double heat_capacity; /* Capacidade térmica média */
	double energy_flux; /* W/m² recebido */

	/* Físico Interno */
	enum bhs_matter_state physical_state; /* Sólido/Liq/Gas */
	bool has_magnetic_field;
};

/**
 * @brief Dados exclusivos de Estrelas
 */
struct bhs_star_data {
	/* Essencial */
	double luminosity; /* Potência total (W) */
	double temp_effective; /* Temperatura efetiva (K) */
	double age; /* Idade (anos) */
	
	/* Físico / Evolutivo */
	enum bhs_star_stage stage;
	double metallicity; /* Fração de metais (Z) */
	char spectral_type[8]; /* ex: "G2V" */
};

/**
 * @brief Dados exclusivos de Buracos Negros
 */
struct bhs_blackhole_data {
	/* Essencial */
	double spin_factor; /* Parâmetro 'a' adimensional (0..1) */
	
	/* Relativístico (Cache) */
	double event_horizon_r; /* Raio de Schwarzschild */
	double ergososphere_r; /* Raio da Ergosfera (equatorial) */

	/* Acoplamento Externo */
	double accretion_disk_mass; /* Massa do disco (kg) */
	double accretion_rate; /* kg/s */
};

/* ============================================================================
 * O CORPO (WRAPPER)
 * ============================================================================
 */

struct bhs_body {
	struct bhs_body_state state; /* Estado Físico Universal */
	enum bhs_body_type type; /* RTTI simples */

	/* Union de Propriedades Específicas */
	union {
		struct bhs_planet_data planet;
		struct bhs_star_data star;
		struct bhs_blackhole_data bh;
	} prop;

	/* 
   * Visual Cache (Separado da física conforme Regra Prática) 
   * "Se não afeta força/estado, não entra no simulador físico"
   * Mas precisamos desenhar, então fica aqui como 'metadata'.
   */
	struct bhs_vec3 color;
};

/* ============================================================================
 * FUNÇÕES (API)
 * ============================================================================
 */

/* Integração de Posição (Step 1 do Verlet) */
void bhs_body_integrate_pos(struct bhs_body *b, double dt);

/* Integração de Velocidade (Step 2 do Verlet) */
void bhs_body_integrate_vel(struct bhs_body *b, double dt);

/* Factories (Helpers para preencher as structs) */
struct bhs_body bhs_body_create_planet_simple(struct bhs_vec3 pos, double mass,
					      double radius,
					      struct bhs_vec3 color);
struct bhs_body bhs_body_create_star_simple(struct bhs_vec3 pos, double mass,
					    double radius,
					    struct bhs_vec3 color);
struct bhs_body bhs_body_create_blackhole_simple(struct bhs_vec3 pos,
						 double mass, double radius);

#endif /* BHS_ENGINE_BODY_H */
