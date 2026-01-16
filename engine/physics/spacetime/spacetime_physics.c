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
 * ============================================================================
 * CONFIGURAÇÃO DA VISUALIZAÇÃO DO ESPAÇO-TEMPO
 * ============================================================================
 *
 * Modo: REALISMO FÍSICO
 *
 * Na física real, o raio de Schwarzschild de planetas é microscópico:
 *   - Sol: rs ≈ 3 km
 *   - Júpiter: rs ≈ 2.8 m
 *   - Terra: rs ≈ 9 mm
 *
 * Portanto, só estrelas e buracos negros criam deformação visível.
 * Planetas são ignorados na visualização (massa < limiar).
 */

/*
 * FLAMM_SCALE: Fator de escala visual para a profundidade do poço.
 * Com Sol (M=20) e rs=40, depth ≈ -SCALE * 40 / (r + 40)
 * Para poço visível, usar 1.0 a 5.0.
 */
#define FLAMM_SCALE 2.0

/*
 * MAX_DEPTH: Profundidade máxima visual (evita infinito).
 */
#define MAX_DEPTH 20.0

/*
 * EPSILON: Suavização mínima (evita singularidade em r=0).
 */
#define EPSILON 1.0

/*
 * MASS_THRESHOLD: Massa mínima para afetar o grid.
 * Com sistema de unidades atual:
 *   - Sol = 20.0 (afeta)
 *   - Júpiter = 0.019 (não afeta)
 *   - Estrelas típicas >= 0.5 (afetam)
 */
#define MASS_THRESHOLD 0.5

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

/*
 * REMOVIDO: planet_dimple_depth()
 *
 * A função "dimple" era uma gambiarra visual que criava
 * um "dente" gaussiano ao invés de usar física real.
 *
 * Problema: Isso violava a proporcionalidade da Relatividade
 * Geral. Se Einstein visse isso, ele viraria no túmulo.
 *
 * SOLUÇÃO: Todos os corpos agora usam flamm_embedding()
 * com rs proporcional à massa, mas com boost visual para
 * corpos pequenos (PLANET_RS_BOOST).
 */

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
	if (!st)
		return;

	const int stride = 6;

	/*
	 * Caso especial: sem corpos ou bodies_ptr nulo.
	 * Reseta o grid para plano.
	 */
	if (!bodies_ptr || n_bodies <= 0) {
		for (int i = 0; i < st->num_vertices; i++) {
			float *v = &st->vertex_data[i * stride];
			v[1] = 0.0f;  /* Y = 0 (plano) */
			/* Cor: ciano neutro */
			v[3] = 0.2f;
			v[4] = 0.9f;
			v[5] = 1.0f;
		}
		return;
	}

	const struct bhs_body *bodies = (const struct bhs_body *)bodies_ptr;

	/*
	 * REALISMO FÍSICO:
	 * Só corpos com massa >= MASS_THRESHOLD (0.5) afetam o grid.
	 * Isso inclui estrelas e buracos negros.
	 * Planetas têm rs microscópico e são ignorados.
	 */
	double rs[64], px[64], pz[64];
	int n_bodies_valid = 0;

	for (int b = 0; b < n_bodies && b < 64; b++) {
		double M = bodies[b].state.mass;

		/* Ignora corpos abaixo do limiar de massa */
		if (M < MASS_THRESHOLD)
			continue;

		px[n_bodies_valid] = bodies[b].state.pos.x;
		pz[n_bodies_valid] = bodies[b].state.pos.z;
		rs[n_bodies_valid] = 2.0 * M;  /* rs = 2M (unidades naturais) */
		n_bodies_valid++;
	}

	/*
	 * Se não há corpos massivos, reseta grid para plano.
	 */
	if (n_bodies_valid == 0) {
		for (int i = 0; i < st->num_vertices; i++) {
			float *v = &st->vertex_data[i * stride];
			v[1] = 0.0f;
			v[3] = 0.2f;
			v[4] = 0.9f;
			v[5] = 1.0f;
		}
		return;
	}

	/*
	 * Itera sobre vértices da malha.
	 */

	for (int i = 0; i < st->num_vertices; i++) {
		float *v = &st->vertex_data[i * stride];

		double x = v[0];
		double z = v[2];

		double total_depth = 0.0;

		/*
		 * Somatório das curvaturas de TODOS os corpos.
		 * Princípio da superposição (aproximação válida
		 * para campos fracos, que é o nosso caso visual).
		 */
		for (int b = 0; b < n_bodies_valid; b++) {
			double dx = x - px[b];
			double dz = z - pz[b];
			double r = sqrt(dx * dx + dz * dz);

			total_depth += flamm_embedding(r, rs[b]);
		}

		/* Clamp para evitar buraco infinito */
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
