/*
 * src/visual/bhs_fabric.c - Implementação do Tecido Espacial (Doppler)
 * 
 * "O espaço-tempo diz à matéria como se mover; 
 * a matéria diz ao espaço-tempo como se curvar." - Wheeler
 *
 * E eu digo à GPU como desenhar linhas tortas.
 */

#include "include/bhs_fabric.h"
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <string.h>  /* memset */

/* 
 * Se o potencial for menor que isso, clampamos.
 * Evita que o vértice vá parar no /dev/null
 */
#define BHS_FABRIC_MAX_DEPTH -2000.0

/* Escala visual do potencial (exagera a deformação para ficar bonito) */
#define BHS_FABRIC_POTENTIAL_SCALE 5.0

struct bhs_fabric *bhs_fabric_create(uint32_t width, uint32_t height,
				     double spacing)
{
	struct bhs_fabric *f;
	uint32_t x, y;
	uint32_t i;

	if (width < 2 || height < 2)
		return NULL; /* Grid de 1 vértice é um ponto, não um tecido */

	/* 1. Aloca a struct principal */
	f = malloc(sizeof(*f));
	if (!f)
		return NULL; /* Sem memória? Em 2026? */

	f->width = width;
	f->height = height;
	f->spacing = spacing;
	f->n_vertices = width * height;
	
	/* GL_LINES requer 2 índices por linha */
	/* Linhas horizontais: (width-1) * height */
	/* Linhas verticais: width * (height-1) */
	uint32_t n_h_lines = (width - 1) * height;
	uint32_t n_v_lines = width * (height - 1);
	f->n_indices = (n_h_lines + n_v_lines) * 2;

	/* 2. Aloca vértices (Bloco único para cache coherency) */
	f->vertices = calloc(f->n_vertices, sizeof(*f->vertices));
	if (!f->vertices)
		goto err_free_struct;

	/* 3. Aloca índices */
	f->indices = malloc(f->n_indices * sizeof(*f->indices));
	if (!f->indices)
		goto err_free_vertices;

	/* 4. Inicializa posições (Grid plana z=0) */
	double offset_x = (width * spacing) / 2.0;
	double offset_y = (height * spacing) / 2.0;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			struct bhs_fabric_vertex *v = &f->vertices[y * width + x];
			v->pos.x = (x * spacing) - offset_x;
			v->pos.y = (y * spacing) - offset_y;
			v->pos.z = 0.0;
			
			/* Inicialmente não deformado */
			v->cur = v->pos;
			v->potential = 0.0;
		}
	}

	/* 5. Preenche índices (Topologia nunca muda, thank god) */
	i = 0;
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			uint32_t current = y * width + x;

			/* Linha Horizontal (conecta com direita) */
			if (x < width - 1) {
				f->indices[i++] = current;
				f->indices[i++] = current + 1;
			}

			/* Linha Vertical (conecta com baixo) */
			if (y < height - 1) {
				f->indices[i++] = current;
				f->indices[i++] = current + width;
			}
		}
	}

	return f;

err_free_vertices:
	free(f->vertices);
err_free_struct:
	free(f);
	return NULL;
}

void bhs_fabric_destroy(struct bhs_fabric *fabric)
{
	if (!fabric)
		return;

	/* Limpa na ordem inversa da criação, só pra garantir */
	if (fabric->indices)
		free(fabric->indices);
	if (fabric->vertices)
		free(fabric->vertices);
	free(fabric);
}

void bhs_fabric_update(struct bhs_fabric *fabric,
		       const struct bhs_body *bodies,
		       uint32_t n_bodies)
{
	uint32_t i, j, x, y;
	const double G_visual = 1.0; 

	if (!fabric)
		return;

	/* 
	 * [PHASE 1] Vertex Displacement (O(V*B))
	 * Calcula o campo escalar Z (Potencial) e a posição deformada.
	 * Otimizado para acesso linear de memória.
	 */
	for (i = 0; i < fabric->n_vertices; i++) {
		struct bhs_fabric_vertex *v = &fabric->vertices[i];
		double total_pot = 0.0;

		/* Reset para posição base de repouso */
		v->cur.x = v->pos.x;
		v->cur.y = v->pos.y;
		/* z será calculado agora */

		if (bodies && n_bodies > 0) {
			for (j = 0; j < n_bodies; j++) {
				const struct bhs_body *b = &bodies[j];
				
				/* 
				 * Distância no plano projetado (Fabric X,Y mapeia para World X,Z)
				 * Em bhs_fabric.h definimos: cur.x = pos.x, cur.y = pos.y.
				 * O "Z" do tecido é a deformação.
				 * Supomos que Bodies.pos.x e Bodies.pos.y são as coordenadas planares 
				 * compatíveis com este espaço.
				 */
				double dx = v->pos.x - b->state.pos.x;
				double dy = v->pos.y - b->state.pos.y;
				
				double r2 = dx*dx + dy*dy;
				
				/* Softening para evitar singularidade em r=0 */
				double r = sqrt(r2 + 0.1); 

				/* 
				 * [Visual Hack] Planetas são muito leves em relação a estrelas.
				 * Na física real, a deformação deles é invisível nessa escala.
				 * Mas o usuário quer ver "bolinhas pesadas".
				 * Então aplicamos um fator de "dramaticidade" na massa visual.
				 */
				double eff_mass = b->state.mass;
				if (b->type == BHS_BODY_PLANET) {
					eff_mass *= 5000.0; /* 5000x mais pesado visualmente */
				}

				/* V = -GM/r */
				total_pot -= (G_visual * eff_mass) / r;
			}
		}

		v->potential = total_pot;

		/* Aplica deformação no eixo Z (Visual Depth) */
		double deform = total_pot * BHS_FABRIC_POTENTIAL_SCALE;
		
		/* Hard Clamp para evitar z-fighting ou estouro de frustum */
		if (deform < BHS_FABRIC_MAX_DEPTH)
			deform = BHS_FABRIC_MAX_DEPTH;

		v->cur.z = deform;
	}

	/* 
	 * [PHASE 2] Normal Calculation (Differential Geometry) - O(V)
	 * Calcula a normal N = normalize(cross(dU, dV))
	 * Usando diferenças finitas centrais onde possível, ou forward/backward nas bordas.
	 */
	uint32_t w = fabric->width;
	uint32_t h = fabric->height;

	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			struct bhs_fabric_vertex *v = &fabric->vertices[y * w + x];
			
			/* Vizinhos (Clamp nas bordas) */
			uint32_t x_L = (x > 0) ? x - 1 : x;
			uint32_t x_R = (x < w - 1) ? x + 1 : x;
			uint32_t y_U = (y > 0) ? y - 1 : y;
			uint32_t y_D = (y < h - 1) ? y + 1 : y;

			struct bhs_fabric_vertex *vL = &fabric->vertices[y * w + x_L];
			struct bhs_fabric_vertex *vR = &fabric->vertices[y * w + x_R];
			struct bhs_fabric_vertex *vU = &fabric->vertices[y_U * w + x];
			struct bhs_fabric_vertex *vD = &fabric->vertices[y_D * w + x];

			/* Vetores tangentes (Finite Difference) */
			/* Tangente U (Horizontal) = R - L */
			double tx_x = vR->cur.x - vL->cur.x;
			double tx_y = vR->cur.y - vL->cur.y;
			double tx_z = vR->cur.z - vL->cur.z;

			/* Tangente V (Vertical) = D - U */
			/* Nota: Y cresce para baixo ou para cima? Depende da inicialização.
			   Assumindo grade cartesiana padrão. */
			double ty_x = vD->cur.x - vU->cur.x;
			double ty_y = vD->cur.y - vU->cur.y;
			double ty_z = vD->cur.z - vU->cur.z;

			/* Cross Product: N = Tx X Ty */
			/*
			 *    |  i   j   k  |
			 *    | tx_x tx_y tx_z |
			 *    | ty_x ty_y ty_z |
			 */
			double nx = tx_y * ty_z - tx_z * ty_y;
			double ny = tx_z * ty_x - tx_x * ty_z;
			double nz = tx_x * ty_y - tx_y * ty_x;

			/* Normalize via Fast Inverse Square Root approximation?
			   Nah, standard sqrt is fast enough in 2026 rendering loops 
			   unless we are doing millons of verts. We have 10k.
			*/
			double len_sq = nx*nx + ny*ny + nz*nz;
			if (len_sq > 1e-8) {
				double inv_len = 1.0 / sqrt(len_sq);
				v->normal.x = nx * inv_len;
				v->normal.y = ny * inv_len;
				v->normal.z = nz * inv_len;
			} else {
				/* Fallback normal pointing UP (Z+) */
				v->normal.x = 0;
				v->normal.y = 0;
				v->normal.z = 1;
			}
		}
	}
}
