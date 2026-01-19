/*
 * bhs_fabric.h - Definições para o sistema de tecido espacial (Doppler)
 *
 * Este arquivo define as estruturas de dados para a visualização da gravidade
 * usando uma malha 2D deformada em 3D (Embedding Diagram).
 *
 * Invariantes Globais:
 * - Toda a alocação é feita na inicialização (pool fixo).
 * - Não há malloc/free no loop de renderização.
 * - Coordenadas Z representam o potencial gravitacional * fator de escala.
 */

#ifndef BHS_FABRIC_H
#define BHS_FABRIC_H

#include <stdint.h>
#include "bhs_types.h"

/*
 * struct bhs_fabric_vertex - Vértice do tecido espacial
 * @pos: Posição original invariável (x, y) no plano. Invariante: z=0 sempre.
 * @cur: Posição atual deformada (x, y, z) para renderização.
 * @potential: Valor escalar do potencial (para coloração shader/heatmap).
 *
 * Cada vértice representa um ponto na grade espacial.
 * A posição 'pos' é a referência de descanso.
 * A posição 'cur' é recalculada a cada frame baseada na métrica.
 */
struct bhs_fabric_vertex {
	struct bhs_vec3 pos;    /* Invar: z=0 sempre (Espaço Paramétrico U,V mapeado em World X,Z) */
	struct bhs_vec3 cur;    /* Deformado pela gravidade (Embeddding Space X,Y,Z) */
	struct bhs_vec3 normal; /* Normal da superfície para iluminação (Calculado via diff geometry) */
	double potential;       /* Potencial gravitacional escalar (Phi) */
};

/*
 * struct bhs_fabric - Gerenciador do tecido
 * @vertices: Array contíguo de vértices.
 * @n_vertices: Tamanho total do array vertices.
 * @indices: Array de índices para renderização (GL_LINES).
 * @n_indices: Tamanho total do array indices.
 * @width: Quantidade de vértices na largura (eixo X).
 * @height: Quantidade de vértices na altura (eixo Y).
 * @spacing: Distância entre nós da grade em estado de repouso.
 *
 * Gerencia a malha completa.
 * A memória para vertices e indices é de propriedade desta struct
 * e deve ser liberada apenas na destruição do objeto.
 */
struct bhs_fabric {
	struct bhs_fabric_vertex *vertices;
	uint32_t n_vertices;

	uint32_t *indices;
	uint32_t n_indices;

	uint32_t width;
	uint32_t height;
	double spacing;
};

/* --- API Pública --- */

/**
 * bhs_fabric_create - Cria e aloca um novo tecido espacial
 * @width: Número de vértices na largura (deve ser > 1)
 * @height: Número de vértices na altura (deve ser > 1)
 * @spacing: Espaçamento entre vértices (unidades do mundo)
 *
 * Aloca memória para vértices e índices e inicializa a grade plana (z=0).
 *
 * Retorna: Ponteiro para nova struct ou NULL em falha de alocação.
 * O chamador é responsável por chamar bhs_fabric_destroy().
 */
struct bhs_fabric *bhs_fabric_create(uint32_t width, uint32_t height,
				     double spacing);

/**
 * bhs_fabric_destroy - Libera memória do tecido
 * @fabric: Ponteiro para o tecido (pode ser NULL)
 */
void bhs_fabric_destroy(struct bhs_fabric *fabric);

/**
 * bhs_fabric_update - Atualiza a deformação da malha
 * @fabric: O tecido a ser atualizado (não nulo)
 * @bodies: Array de corpos massivos (não nulo se n_bodies > 0)
 * @n_bodies: Número de corpos
 *
 * Recalcula a coordenada Z de cada vértice baseado no potencial gravitacional
 * newtoniano (ou métrica de Schwarzschild aproximada) dos corpos fornecidos.
 *
 * Complexidade: O(V * B) onde V = vértices, B = corpos.
 */
void bhs_fabric_update(struct bhs_fabric *fabric, const struct bhs_body *bodies,
		       uint32_t n_bodies);

/**
 * bhs_fabric_set_spacing - Atualiza o espaçamento entre vértices (zoom)
 * @fabric: O tecido
 * @new_spacing: Novo espaçamento (> 0.0)
 *
 * Recalcula as posições de repouso (pos) e reinicia o estado atual (cur)
 * para a nova grade plana, preservando a topologia.
 */
void bhs_fabric_set_spacing(struct bhs_fabric *fabric, double new_spacing);

#endif /* BHS_FABRIC_H */
