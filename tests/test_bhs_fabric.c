/*
 * tests/test_bhs_fabric.c - Test Harness para o Doppler Fabric
 * 
 * Verificação Constitucional:
 * - Compila sem warnings?
 * - Passa sem segfault?
 * - O código faz o que diz que faz?
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "bhs_fabric.h"

#define ASSERT_NEAR(a, b, epsilon) \
	do { \
		if (fabs((a) - (b)) > (epsilon)) { \
			fprintf(stderr, "FAIL: %f != %f at line %d\n", \
				(double)(a), (double)(b), __LINE__); \
			exit(1); \
		} \
	} while (0)

int main(void) {
	printf("[TEST] Iniciando verificacao do Doppler Fabric...\n");

	/* Teste 1: Criacao */
	printf("[TEST] bhs_fabric_create...\n");
	struct bhs_fabric *f = bhs_fabric_create(10, 10, 1.0);
	assert(f != NULL);
	assert(f->n_vertices == 100);
	assert(f->vertices != NULL);
	assert(f->indices != NULL);
	
	/* Verifica posicoes iniciais (z=0) e centralizacao */
	/* Grid 10x10 com spacing 1.0 -> width=10.0, height=10.0 */
	/* Offset deve ser 5.0 para centralizar */
	struct bhs_fabric_vertex *v_center = &f->vertices[55]; // (5,5) approx
	ASSERT_NEAR(v_center->pos.z, 0.0, 0.001);

	/* Teste 2: Update com 0 corpos */
	printf("[TEST] bhs_fabric_update (0 corpos)...\n");
	bhs_fabric_update(f, NULL, 0);
	/* Deve permanecer z=0 */
	ASSERT_NEAR(f->vertices[0].cur.z, 0.0, 0.001);

	/* Teste 3: Update com 1 corpo no centro */
	printf("[TEST] bhs_fabric_update (1 corpo)...\n");
	struct bhs_body b1 = {
		.position = {0, 0, 0},
		.mass = 100.0,
		.radius = 1.0
	};
	
	/* O vertice mais proximo do centro deve afundar */
	/* (5,5) deve estar perto de 0,0 se o grid for par, ou (4,4) se impar... 
	 * Com 10x10, offset eh 5.0. 
	 * v[0] eh -5,-5. v[55] eh x=0.0->0.5? Vamos achar o mais proximo.
	 */
	bhs_fabric_update(f, &b1, 1);
	
	int found_sink = 0;
	double min_z = 0.0;
	for(uint32_t i=0; i<f->n_vertices; i++) {
		if (f->vertices[i].cur.z < -0.1) {
			found_sink = 1;
			if (f->vertices[i].cur.z < min_z)
				min_z = f->vertices[i].cur.z;
		}
	}
	
	if (!found_sink) {
		fprintf(stderr, "FAIL: Nenhum vertice afundou com massa 100.0!\n");
		exit(1);
	}
	printf("[PASS] Vertice mais fundo: %f\n", min_z);

	/* Teste 4: Cleanup */
	printf("[TEST] bhs_fabric_destroy...\n");
	bhs_fabric_destroy(f);
	
	printf("[SUCCESS] Todos os testes passaram sem explodir.\n");
	return 0;
}
