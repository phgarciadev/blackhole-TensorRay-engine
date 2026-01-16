/**
 * @file spacetime_physics.c
 * @brief Deformação Visual do Espaço-Tempo (Curvatura Gravitacional)
 *
 * "O espaço diz à matéria como mover-se; a matéria diz ao espaço como curvar-se."
 * - John Archibald Wheeler
 *
 * ============================================================================
 * FÍSICA IMPLEMENTADA: EMBEDDING DIAGRAM DE FLAMM
 * ============================================================================
 *
 * O embedding diagram é a forma CORRETA de visualizar a curvatura do espaço-tempo
 * em Relatividade Geral. Foi derivado por Ludwig Flamm em 1916.
 *
 * A métrica de Schwarzschild (para espaço-tempo esfericamente simétrico):
 *   ds² = -(1 - rₛ/r)dt² + dr²/(1 - rₛ/r) + r²dΩ²
 *
 * onde rₛ = 2GM/c² é o raio de Schwarzschild.
 *
 * O embedding em 3D (com θ = π/2, superfície equatorial):
 *   z(r) = 2√(rₛ(r - rₛ))  para r ≥ rₛ
 *
 * Este é um PARABOLOIDE que representa como o espaço está "esticado"
 * perto de uma massa.
 *
 * ============================================================================
 * IMPLEMENTAÇÃO
 * ============================================================================
 *
 * 1. Para BURACOS NEGROS e ESTRELAS (M ≥ threshold):
 *    Usa a fórmula de Flamm: z = 2√(rₛ(r - rₛ))
 *    Invertido (para baixo) para visualização.
 *
 * 2. Para PLANETAS (M < threshold):
 *    Usa um "dimple" gaussiano proporcional ao raio físico.
 *    Isso torna planetas visíveis sem distorcer a física macro.
 *
 * ============================================================================
 * REFERÊNCIAS
 * ============================================================================
 *
 * - Flamm, L. (1916). Beiträge zur Einsteinschen Gravitationstheorie
 * - Misner, Thorne, Wheeler (1973). Gravitation, Box 23.1
 * - Marolf, D. (1999). "Spacetime Embedding Diagrams for Black Holes"
 * - Stanley kubrick forjando o pouso na lua (todos sabemos que ele fez isso)
 * - Cristian bale achando que interestelar é bom por causa dele (e não pelos 15 cientistas e estudiosos que ele contratou)
 */

#include "engine/scene/scene.h"
#include "spacetime_internal.h"
#include <math.h>

/*
 * ============================================================================
 * CONSTANTES DE ESCALA VISUAL
 * ============================================================================
 *
 * Esses valores controlam a aparência do "poço gravitacional".
 * O objetivo é criar uma deformação VISÍVEL semelhante aos diagramas clássicos.
 */

/*
 * Escala de profundidade para o embedding.
 * Valor ALTO = poço mais profundo e visível.
 * Com M☉ = 20, rs = 40, SCALE = 8 → depth máx ≈ 4 unidades
 */
#define FLAMM_SCALE 8.0

/*
 * Limiar de massa para considerar "corpo massivo" (estrela, BH).
 */
#define MASS_THRESHOLD 0.5

/*
 * Escala para indicador visual de planetas.
 */
#define PLANET_DIMPLE_SCALE 3.0

/*
 * Raio de influência do dimple planetário.
 */
#define PLANET_INFLUENCE_RADIUS 10.0

/*
 * Profundidade MÁXIMA visual (limite de segurança).
 */
#define MAX_DEPTH 15.0

/*
 * Suavização mínima.
 */
#define EPSILON 0.5

/**
 * flamm_embedding - Calcula profundidade usando o Embedding de Flamm
 * @r: Distância radial do centro de massa
 * @rs: Raio de Schwarzschild (2M em unidades naturais)
 *
 * Fórmula de Flamm: z(r) = 2 * sqrt(rs * (r - rs))
 *
 * Esta é a forma CORRETA de visualizar a curvatura do espaço-tempo
 * em relatividade geral. O paraboloide resultante mostra como
 * o espaço é "esticado" perto de uma massa.
 *
 * Para r → ∞, z → ∞ (espaço plano no infinito)
 * Para r = rs, z = 0 (horizonte de eventos)
 * Para r < rs, a fórmula não se aplica (dentro do horizonte)
 */
static inline double flamm_embedding(double r, double rs)
{
	if (rs <= 0.0)
		return 0.0;

	/*
	 * Simplificação visual do embedding de Flamm.
	 * 
	 * Em vez da fórmula matemática exata (que vai a infinito),
	 * usamos um perfil suave que decai com a distância:
	 *
	 *   depth = -SCALE * rs / (r + rs)
	 *
	 * Isso produz:
	 *   - Poço máximo perto do centro: -SCALE * rs / rs = -SCALE
	 *   - Decai suavemente para zero em r → ∞
	 *   - Não tem paredes verticais
	 */
	if (r < EPSILON) {
		r = EPSILON;
	}

	double depth = -FLAMM_SCALE * rs / (r + rs);

	/* Limita profundidade máxima */
	if (depth < -MAX_DEPTH)
		depth = -MAX_DEPTH;

	return depth;
}

/**
 * planet_dimple_depth - Calcula indicador visual para planeta
 * @r: Distância radial do centro do planeta
 * @radius: Raio físico do planeta
 *
 * Cria um pequeno "dente" visível ao redor do planeta.
 * Perfil gaussiano: exp(-r²/σ²)
 */
static inline double planet_dimple_depth(double r, double radius)
{
	if (radius <= 0.0)
		return 0.0;

	double influence = radius * PLANET_INFLUENCE_RADIUS;

	if (r > influence)
		return 0.0;

	double sigma_sq = influence * influence;
	double gaussian = exp(-(r * r) / sigma_sq);

	/* Profundidade proporcional ao raio do planeta */
	return -radius * PLANET_DIMPLE_SCALE * gaussian;
}

/**
 * redshift_color - Calcula cor baseada no redshift gravitacional
 * @depth: Profundidade no poço gravitacional
 * @r: Red output
 * @g: Green output
 * @b: Blue output
 *
 * Em relatividade geral, luz escalando um poço gravitacional sofre redshift:
 *   λ_obs/λ_emit = 1/sqrt(1 - rs/r)
 *
 * Simplificamos para um gradiente visual:
 *   - Raso (z≈0): Ciano (espaço "frio")
 *   - Profundo: Vermelho (redshift extremo)
 */
static inline void redshift_color(double depth, float *r, float *g, float *b)
{
	/* Normaliza profundidade [0, 1] */
	float depth_norm = (float)(-depth / MAX_DEPTH);
	if (depth_norm < 0.0f) depth_norm = 0.0f;
	if (depth_norm > 1.0f) depth_norm = 1.0f;

	/*
	 * Paleta de cores inspirada em visualizações científicas:
	 *   depth=0 (raso):   ciano brilhante (0.2, 0.9, 1.0)
	 *   depth=0.5:        azul-violeta
	 *   depth=1 (profundo): vermelho-magenta (0.9, 0.1, 0.3)
	 */
	*r = 0.2f + depth_norm * 0.7f;
	*g = 0.9f - depth_norm * 0.8f;
	*b = 1.0f - depth_norm * 0.7f;
}

void bhs_spacetime_update(bhs_spacetime_t st, const void *bodies_ptr,
                          int n_bodies)
{
	if (!st || !bodies_ptr || n_bodies <= 0)
		return;

	const struct bhs_body *bodies = (const struct bhs_body *)bodies_ptr;

	/*
	 * Separa corpos em duas categorias:
	 * - Massivos: usam embedding de Flamm (fisica real)
	 * - Leves (planetas): usam dimple visual
	 */
	double rs_m[64], px_m[64], pz_m[64];
	int n_massive = 0;

	double radius_p[64], px_p[64], pz_p[64];
	int n_planet = 0;

	for (int b = 0; b < n_bodies && b < 64; b++) {
		double M = bodies[b].state.mass;
		double R = bodies[b].state.radius;
		double px = bodies[b].state.pos.x;
		double pz = bodies[b].state.pos.z;

		if (M <= 0)
			continue;

		if (M >= MASS_THRESHOLD) {
			/* Corpo massivo: estrela, buraco negro */
			rs_m[n_massive] = 2.0 * M;  /* Raio de Schwarzschild */
			px_m[n_massive] = px;
			pz_m[n_massive] = pz;
			n_massive++;
		} else {
			/* Planeta: usa indicador visual */
			radius_p[n_planet] = R;
			px_p[n_planet] = px;
			pz_p[n_planet] = pz;
			n_planet++;
		}
	}

	if (n_massive == 0 && n_planet == 0)
		return;

	/*
	 * Itera sobre vértices da malha.
	 * Stride = 6 floats: x, y, z, r, g, b
	 */
	int stride = 6;

	for (int i = 0; i < st->num_vertices; i++) {
		float *v = &st->vertex_data[i * stride];

		double x = v[0];
		double z = v[2];

		double total_depth = 0.0;

		/*
		 * 1. Contribuição de corpos massivos (Flamm embedding).
		 */
		for (int b = 0; b < n_massive; b++) {
			double dx = x - px_m[b];
			double dz = z - pz_m[b];
			double r = sqrt(dx * dx + dz * dz);

			total_depth += flamm_embedding(r, rs_m[b]);
		}

		/*
		 * 2. Contribuição de planetas (dimple visual).
		 */
		for (int p = 0; p < n_planet; p++) {
			double dx = x - px_p[p];
			double dz = z - pz_p[p];
			double r = sqrt(dx * dx + dz * dz);

			total_depth += planet_dimple_depth(r, radius_p[p]);
		}

		/* Clamp */
		if (total_depth < -MAX_DEPTH)
			total_depth = -MAX_DEPTH;

		v[1] = (float)total_depth;

		/* Cor baseada na profundidade (redshift gravitacional) */
		redshift_color(total_depth, &v[3], &v[4], &v[5]);
	}
}

float bhs_spacetime_get_depth_at_point(bhs_spacetime_t st, float x, float z) {
    if (!st || !st->vertex_data) return 0.0f;

    /*
     * Bilinear Interpolation from the Grid.
     * Maps physical coordinates (x, z) to grid indices.
     */
    float half_size = st->size * 0.5f;
    float dx = x + half_size; // 0..size
    float dz = z + half_size; // 0..size
    
    if (dx < 0 || dx >= st->size || dz < 0 || dz >= st->size) return 0.0f;

    float cell_size = st->size / st->divisions;
    
    // Normalized coords
    float fx = dx / cell_size;
    float fz = dz / cell_size;
    
    int ix = (int)fx;
    int iz = (int)fz;
    
    if (ix < 0 || ix >= st->divisions || iz < 0 || iz >= st->divisions) return 0.0f;
    
    // Grid structure: (Map 2D -> 1D array)
    // Vertices stored as (x,y,z, r,g,b)
    int stride = 6;
    int cols = st->divisions + 1;
    
    // Indices for 4 neighbors
    int i00 = (iz * cols + ix) * stride;
    int i10 = (iz * cols + (ix + 1)) * stride;
    int i01 = ((iz + 1) * cols + ix) * stride;
    int i11 = ((iz + 1) * cols + (ix + 1)) * stride;
    
    // Y values (Depth)
    float y00 = st->vertex_data[i00 + 1];
    float y10 = st->vertex_data[i10 + 1];
    float y01 = st->vertex_data[i01 + 1];
    float y11 = st->vertex_data[i11 + 1];
    
    float u = fx - ix;
    float v = fz - iz;
    
    // Interpolate
    float y0 = y00 * (1.0f - u) + y10 * u;
    float y1 = y01 * (1.0f - u) + y11 * u;
    
    return y0 * (1.0f - v) + y1 * v;
}
