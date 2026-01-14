/**
 * @file body.h
 * @brief Estruturas de Dados Físicas (Kernel Style)
 *
 * "Dados dominam. Se você conhece os dados, a lógica é óbvia."
 * - Torvalds (adaptado)
 *
 * ============================================================================
 * ⚠️  DEPRECATED - NÃO USE PARA CÓDIGO NOVO ⚠️
 * ============================================================================
 *
 * Este arquivo contém uma struct monolítica que viola princípios ECS
 * e causa duplicação de dados com os componentes em components.h.
 *
 * Para código novo, use os componentes granulares:
 *   - bhs_transform_component (posição, rotação)
 *   - bhs_physics_component (velocidade, massa, forças)
 *   - bhs_celestial_component (dados astrofísicos)
 *   - bhs_collider_component (forma de colisão)
 *   - bhs_kerr_metric_component (relatividade)
 *
 * Migração:
 *   struct bhs_body  →  Entidade ECS + Componentes
 *   bhs_body_integrate()  →  bhs_physics_system_update()
 *   bhs_body_create_*()  →  bhs_ecs_create_entity() + add_components
 *
 * Este arquivo será REMOVIDO na próxima versão major.
 */

#ifndef BHS_ENGINE_BODY_H
#define BHS_ENGINE_BODY_H

/*
 * DEPRECATED: Este header será removido. Veja comentário no topo do arquivo.
 */

#include "lib/math/vec4.h"

struct bhs_planet_desc; /* Forward declaracao */

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
	BHS_BODY_MOON,       /* Satélite natural */
	BHS_BODY_STAR,
	BHS_BODY_BLACKHOLE,
	BHS_BODY_ASTEROID,   /* Corpos menores */
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
	double luminosity;      /* Potência total (W) */
	double temp_effective;  /* Temperatura efetiva (K) */
	double age;             /* Idade (anos) */
	double density;         /* Densidade média (kg/m³) */
	
	/* Composição */
	double hydrogen_frac;   /* Fração de hidrogênio (0..1) */
	double helium_frac;     /* Fração de hélio (0..1) */
	double metals_frac;     /* Fração de metais (Z) */

	/* Evolutivo */
	enum bhs_star_stage stage;
	double metallicity;     /* Metalicidade [Fe/H] */
	char spectral_type[8];  /* ex: "G2V" */
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

	/* Flags de Controle Físico */
	bool is_fixed;    /* Se true, não se move (massa infinita efetiva) */
	bool is_alive;    /* Se false, foi absorvido/destruído */
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

/**
 * @brief Cria um corpo a partir do descritor detalhado (New System)
 */
struct bhs_body bhs_body_create_from_desc(const struct bhs_planet_desc *desc, 
					  struct bhs_vec3 pos);

#endif /* BHS_ENGINE_BODY_H */
