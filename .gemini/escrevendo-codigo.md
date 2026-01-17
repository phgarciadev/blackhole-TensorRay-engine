# 📜 Escrevendo Código - Guia Definitivo

Este documento é **LEITURA OBRIGATÓRIA** antes de escrever qualquer linha de código neste projeto. Se você pulou pra cá sem ler, volte e leia o `GEMINI.md` primeiro. Eu espero. Nao use emojis, coisa de geti mediokre

---

## Sumário

1. [Filosofia Geral](#1-filosofia-geral)
2. [Linguagem C - Padrão Kernel Linux](#2-linguagem-c---padrão-kernel-linux)
3. [Linguagem Rust - Padrão Kernel Linux](#3-linguagem-rust---padrão-kernel-linux)
4. [Exceção: C++ Moderno](#4-exceção-c-moderno)
5. [Exceção: Objective-C++](#5-exceção-objective-c-arquivos-mm)
6. [Comentários e Documentação](#6-comentários-e-documentação)
7. [Tratamento de Erros](#7-tratamento-de-erros)
8. [Performance e Algoritmos](#8-performance-e-algoritmos)
9. [Organização de Arquivos](#9-organização-de-arquivos)
10. [Invariantes e Contratos](#10-invariantes-e-contratos)
11. [Checklist Antes de Commitar](#11-checklist-antes-de-commitar)

---

## 1. Filosofia Geral

### 1.1 Código é Comunicação

Código não é escrito para computadores. Computadores entendem binário. Código é escrito para **humanos** lerem. Se outro desenvolvedor não entende seu código em 30 segundos, você falhou.

### 1.2 Simplicidade Radical

> "A perfeição é alcançada não quando não há mais nada a adicionar, mas quando não há mais nada a remover."
> — Antoine de Saint-Exupéry (e provavelmente Linus em algum e-mail furioso)

- **Menos código** = menos bugs
- **Menos abstrações** = menos indireções = mais velocidade
- **Menos dependências** = menos problemas

### 1.3 Performance por Padrão

Não otimize prematuramente, mas também não escreva código lento por preguiça. Pense em:

- Cache locality (dados próximos na memória)
- Branch prediction (evite ifs dentro de loops quentes)
- Alocações (prefira stack, evite heap quando possível)

---

## 2. Linguagem C - Padrão Kernel Linux

### 2.1 Estilo de Formatação

```c
/* 
 * CORRETO: Estilo K&R, tabs de 8 espaços (ou tab literal)
 */
int funcao_exemplo(int parametro)
{
	if (parametro < 0) {
		return -EINVAL;
	}

	for (int i = 0; i < parametro; i++) {
		fazer_algo(i);
	}

	return 0;
}

/*
 * ERRADO: Estilo Allman, espaços, chaves em linha separada pra if
 */
int funcao_errada(int parametro)
{
    if (parametro < 0)
    {
        return -1;
    }
    return 0;
}
```

### 2.2 Nomenclatura

| Tipo | Convenção | Exemplo |
|------|-----------|---------|
| Funções | snake_case com prefixo do módulo | `bhs_mesh_create()` |
| Variáveis | snake_case curto | `int count`, `struct bhs_vec3 pos` |
| Constantes | SCREAMING_SNAKE_CASE | `#define BHS_MAX_BODIES 1024` |
| Structs | `struct bhs_nome` | `struct bhs_body` |
| Enums | `enum bhs_nome` + `BHS_NOME_VALOR` | `enum bhs_state { BHS_STATE_INIT }` |
| Handles (opaco) | `typedef struct impl *nome_t` | `typedef struct bhs_mesh_impl *bhs_mesh_t` |

### 2.3 Proibições Absolutas

❌ **NUNCA FAÇA:**

```c
/* Proibido: typedef de struct (exceto handles opacos) */
typedef struct {
	int x, y;
} Ponto;  /* ERRADO */

struct ponto {
	int x, y;
};  /* CORRETO */

/* Proibido: malloc sem checagem */
void *ptr = malloc(size);
usar(ptr);  /* ERRADO: pode ser NULL */

void *ptr = malloc(size);
if (!ptr)
	return -ENOMEM;
usar(ptr);  /* CORRETO */

/* Proibido: variáveis não inicializadas */
int valor;
usar(valor);  /* ERRADO: lixo de memória */

int valor = 0;  /* CORRETO */

/* Proibido: números mágicos */
if (x > 3.14159) { }  /* ERRADO */

#define BHS_PI 3.14159265358979323846
if (x > BHS_PI) { }  /* CORRETO */
```

### 2.4 Structs e Inicialização

```c
/* Declaração de struct */
struct bhs_body {
	struct bhs_vec3 pos;
	struct bhs_vec3 vel;
	double mass;
	double radius;
	enum bhs_body_type type;
};

/* Inicialização designada (sempre!) */
struct bhs_body planeta = {
	.pos = { .x = 0, .y = 0, .z = 0 },
	.vel = { .x = 1, .y = 0, .z = 0 },
	.mass = 1.0,
	.radius = 0.5,
	.type = BHS_BODY_PLANET,
};

/* Zerando struct */
struct bhs_body vazio = { 0 };  /* ou memset */
```

### 2.5 Ponteiros e Memória

```c
/*
 * Regras de ponteiros:
 * 1. Ponteiro de entrada: const se não modifica
 * 2. Ponteiro de saída: documentar quem libera
 * 3. Sempre verificar NULL
 */

/* Entrada read-only */
double bhs_body_energia(const struct bhs_body *b)
{
	if (!b)
		return 0.0;
	return 0.5 * b->mass * bhs_vec3_dot(b->vel, b->vel);
}

/* Saída: chamador libera */
struct bhs_body *bhs_body_criar(double mass)
{
	struct bhs_body *b = malloc(sizeof(*b));
	if (!b)
		return NULL;
	
	*b = (struct bhs_body){ .mass = mass };
	return b;
}

void bhs_body_destruir(struct bhs_body *b)
{
	free(b);  /* free(NULL) é seguro */
}
```

### 2.6 Headers e Includes

```c
/* bhs_modulo.h */
#ifndef BHS_MODULO_H
#define BHS_MODULO_H

#include <stdint.h>     /* Headers do sistema primeiro */
#include <stdbool.h>

#include "bhs_types.h"  /* Depois headers do projeto */

/* Declarações públicas aqui */

#endif /* BHS_MODULO_H */
```

**Ordem de includes:**
1. Header correspondente (se for .c implementando .h)
2. Headers do sistema (`<stdio.h>`, `<stdlib.h>`)
3. Headers do projeto (`"bhs_*.h"`)

### 2.7 Macros

```c
/* Macros simples: parênteses sempre */
#define BHS_MIN(a, b)  ((a) < (b) ? (a) : (b))
#define BHS_MAX(a, b)  ((a) > (b) ? (a) : (b))
#define BHS_CLAMP(x, lo, hi)  BHS_MIN(BHS_MAX(x, lo), hi)

/* Macros multi-linha: do { } while(0) */
#define BHS_LOG_ERRO(fmt, ...) \
	do { \
		fprintf(stderr, "[ERRO] " fmt "\n", ##__VA_ARGS__); \
	} while (0)

/* Macro de tamanho de array */
#define BHS_ARRAY_SIZE(arr)  (sizeof(arr) / sizeof((arr)[0]))
```

---

## 3. Linguagem Rust - Padrão Kernel Linux

### 3.1 Configuração Básica

Todo arquivo Rust DEVE começar com:

```rust
//! Descrição do módulo em português
//!
//! Este módulo faz X, Y e Z.

#![no_std]
#![no_main]  // se for entry point

// Sem crates externas!
// Sem cargo!
```

### 3.2 Tipos e Estruturas

```rust
/// Vetor 3D para cálculos espaciais
#[repr(C)]  // Compatibilidade FFI
pub struct Vec3 {
    pub x: f64,
    pub y: f64,
    pub z: f64,
}

impl Vec3 {
    /// Cria vetor zero
    pub const fn zero() -> Self {
        Self { x: 0.0, y: 0.0, z: 0.0 }
    }

    /// Produto escalar
    pub fn dot(&self, other: &Self) -> f64 {
        self.x * other.x + self.y * other.y + self.z * other.z
    }
}
```

### 3.3 FFI com C

```rust
/// Interface FFI para código C
/// 
/// # Segurança
/// O chamador deve garantir que `ptr` é válido e alinhado.
#[no_mangle]
pub unsafe extern "C" fn bhs_vec3_dot(a: *const Vec3, b: *const Vec3) -> f64 {
    // SAFETY: Documentado que chamador garante validade
    let a = unsafe { &*a };
    let b = unsafe { &*b };
    a.dot(b)
}
```

### 3.4 Tratamento de Erros (sem panic!)

```rust
/// Resultado de operações que podem falhar
#[repr(C)]
pub enum BhsResult {
    Ok = 0,
    ErrInvalid = -1,
    ErrOverflow = -2,
}

/// Divisão segura (nunca faz panic)
pub fn div_seguro(a: f64, b: f64) -> Result<f64, BhsResult> {
    if b.abs() < 1e-15 {
        Err(BhsResult::ErrInvalid)
    } else {
        Ok(a / b)
    }
}
```

### 3.5 Proibições em Rust

❌ **NUNCA:**
- `std::*` (estamos em `no_std`)
- `panic!()`, `unwrap()`, `expect()` em código de produção
- Crates externas
- `cargo` para build (use Makefile)
- Alocação dinâmica sem `alloc` explícito

---

## 4. Exceção: C++ Moderno

Arquivos `.cpp` são **exceções conscientes**. Quando você decide usar C++, é porque precisa de features específicas.

### 4.1 Versão e Features

```cpp
// Mínimo C++17, preferência C++20
// Compile com: -std=c++20 -fno-exceptions -fno-rtti

#include <memory>
#include <optional>
#include <span>
#include <expected>  // C++23
```

### 4.2 RAII Obrigatório

```cpp
// ERRADO: ponteiro cru com new/delete
void funcao_ruim() {
    auto* ptr = new Recurso();
    usar(ptr);
    delete ptr;  // E se usar() lançar exceção? Leak!
}

// CORRETO: smart pointer
void funcao_boa() {
    auto ptr = std::make_unique<Recurso>();
    usar(*ptr);
}  // Destrutor automático
```

### 4.3 Sem Exceptions

```cpp
// Use std::expected ou códigos de erro
std::expected<Resultado, Erro> funcao_que_pode_falhar() {
    if (algo_deu_errado)
        return std::unexpected(Erro::Invalido);
    return Resultado{};
}
```

---

## 5. Exceção: Objective-C++ (arquivos `.mm`)

Mesma filosofia do C++: se existe um arquivo `.mm`, é porque **precisamos** conversar com APIs da Apple (Cocoa, Metal, etc). Não é nosso foco, aparece em lugares muito específicos onde a escolha mais inteligente é essa linguagem híbrida.

### 5.1 Quando Usar

- Backends macOS/iOS (Cocoa, AppKit, UIKit)
- Integração com Metal
- Qualquer coisa que precise de `@interface`, `@implementation`, ou colchetes estranhos

### 5.2 Regras

```objc
// ARC é obrigatório - ninguém merece retain/release manual
// Compile com: -fobjc-arc -std=c++20

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>

// Mistura C++ com ObjC sem medo (mas com cuidado)
struct MeuEstadoCpp {
    std::vector<int> dados;
    NSWindow *janela;  // Ponteiro ObjC dentro de struct C++
};
```

### 5.3 Práticas

1. **Use `@autoreleasepool`** em loops e funções que criam muitos objetos ObjC
2. **`__bridge`** para converter entre ponteiros C e ObjC quando necessário
3. **Prefira `nil`** para ponteiros ObjC, `nullptr` para ponteiros C/C++
4. **Documente a bagunça** - Misturar duas linguagens é confuso, comente bem

### 5.4 Estrutura Típica

```objc
/* Interface pública é C puro (no lib.h) */
extern "C" {
#include "ux/platform/platform.h"
}

/* Internamente, a festa do Objective-C++ */
@interface BHSMinhaClasse : NSObject
@property (nonatomic, assign) struct meu_estado_c *impl;
@end

@implementation BHSMinhaClasse
- (void)metodoObjC {
    // Pode usar C++ aqui dentro, vai fundo
    self.impl->dados.push_back(42);
}
@end

/* Implementação da API C usando o ObjC internamente */
extern "C" int bhs_funcao_publica(void) {
    @autoreleasepool {
        BHSMinhaClasse *obj = [[BHSMinhaClasse alloc] init];
        // faz a magia
        return 0;
    }
}
```

A ideia é: **a API pública é C**, o ObjC++ fica escondido na implementação. Quem usa a lib nem sabe que tem Objective-C por baixo.

---

## 6. Comentários e Documentação

### 6.1 Idioma

**TUDO EM PORTUGUÊS BRASILEIRO.** Exceto:
- Identificadores de código (`bhs_mesh_create`, não `bhs_malha_criar`)
- Termos técnicos sem tradução boa (shader, buffer, cache)
- Muita ironia e humor acido, mas sem exagerar. Você tem personalidade demais.
- Não leve os exemplos abaixo tão a serio em questão de tamanho, comentários muito grandes são ruins. 

### 6.2 Formato de Comentários

```c
/*
 * Comentário de bloco para explicações longas.
 * Cada linha começa com asterisco alinhado.
 * 
 * Use para:
 * - Explicar algoritmos complexos
 * - Documentar invariantes
 * - Avisos importantes
 */

/* Comentário de linha única para coisas simples */

// Estilo C++ (permitido em C99+, mas evite em headers públicos)
```

### 6.3 Documentação de Funções

```c
/**
 * bhs_geodesica_integrar - Integra geodésica por um passo de tempo
 * @geo: Ponteiro para estrutura da geodésica (não nulo)
 * @dt: Passo de tempo em unidades naturais (positivo)
 *
 * Integra as equações de geodésica usando Runge-Kutta 4ª ordem.
 * A métrica é obtida do campo escalar global.
 *
 * Invariantes:
 * - @geo->pos permanece dentro do domínio computacional
 * - @geo->vel permanece normalizado (para geodésicas nulas)
 *
 * Retorna (assim esperamos):
 * - 0 em sucesso
 * - -EINVAL se parâmetros inválidos
 * - -ERANGE se saiu do domínio
 */
int bhs_geodesica_integrar(struct bhs_geodesica *geo, double dt);
```

### 6.4 TODOs e FIXMEs

```c
/* TODO(pedro): Implementar caso de buraco negro rotativo */
/* FIXME: Esse cálculo estoura com massas > 1e10 */
/* HACK: Gambiarra temporária até resolver o issue #42 */
/* XXX: Isso aqui é feio mas funciona - revisar depois */
```

---

## 7. Tratamento de Erros

### 7.1 Códigos de Erro

```c
/* Use valores negativos para erros, 0 para sucesso */
enum bhs_erro {
	BHS_OK = 0,
	BHS_ERR_NOMEM = -1,      /* Sem memória */
	BHS_ERR_INVALIDO = -2,   /* Parâmetro inválido */
	BHS_ERR_NAOENC = -3,     /* Não encontrado */
	BHS_ERR_OVERFLOW = -4,   /* Overflow numérico */
	BHS_ERR_DOMINIO = -5,    /* Fora do domínio válido */
};
```

### 7.2 Padrão de Checagem

```c
int bhs_operacao_complexa(struct bhs_ctx *ctx, int param)
{
	int ret;

	/* Validação de entrada primeiro */
	if (!ctx)
		return BHS_ERR_INVALIDO;
	if (param < 0 || param > BHS_PARAM_MAX)
		return BHS_ERR_INVALIDO;

	/* Operações que podem falhar */
	ret = passo_um(ctx);
	if (ret < 0)
		return ret;  /* Propaga erro */

	ret = passo_dois(ctx, param);
	if (ret < 0)
		goto erro_cleanup;  /* Limpa antes de sair */

	return BHS_OK;

erro_cleanup:
	desfazer_passo_um(ctx);
	return ret;
}
```

### 7.3 Goto para Cleanup (Sim, é Permitido)

No kernel Linux, `goto` é usado extensivamente para cleanup. É **correto** e **legível**:

```c
int bhs_recurso_inicializar(struct bhs_recurso *r)
{
	r->buffer = malloc(BUFFER_SIZE);
	if (!r->buffer)
		return -ENOMEM;

	r->tabela = malloc(TABELA_SIZE);
	if (!r->tabela)
		goto erro_buffer;

	r->contexto = criar_contexto();
	if (!r->contexto)
		goto erro_tabela;

	return 0;

erro_tabela:
	free(r->tabela);
erro_buffer:
	free(r->buffer);
	return -ENOMEM;
}
```

---

## 8. Performance e Algoritmos

### 8.1 Complexidade Aceitável

| Situação | Complexidade Máxima | Nota |
|----------|---------------------|------|
| Loop principal | O(n) | n = número de corpos |
| Busca | O(log n) | Use estruturas ordenadas |
| Inicialização | O(n²) | Só na startup |
| Renderização | O(vértices) | GPU faz o trabalho pesado |

### 8.2 Otimizações Esperadas

```c
/* RUIM: Divisão dentro do loop */
for (int i = 0; i < n; i++) {
	resultado[i] = valores[i] / constante;
}

/* BOM: Multiplicação por inverso */
double inv = 1.0 / constante;
for (int i = 0; i < n; i++) {
	resultado[i] = valores[i] * inv;
}

/* RUIM: Cálculo repetido */
for (int i = 0; i < n; i++) {
	double r = sqrt(x*x + y*y + z*z);
	usar(r);
}

/* BOM: Calcular uma vez se não muda */
double r = sqrt(x*x + y*y + z*z);
for (int i = 0; i < n; i++) {
	usar(r);
}
```

### 8.3 Cache e Memória

```c
/* RUIM: Acesso aleatório (cache miss) */
for (int i = 0; i < n; i++) {
	processar(lista_encadeada->proximo);
}

/* BOM: Array contíguo (cache friendly) */
for (int i = 0; i < n; i++) {
	processar(&array[i]);
}

/* Estrutura orientada a dados (SoA vs AoS) */

/* AoS - Array of Structures (às vezes ruim) */
struct Particula { float x, y, z, vx, vy, vz; };
struct Particula particulas[1000];

/* SoA - Structure of Arrays (melhor pra SIMD) */
struct Particulas {
	float x[1000], y[1000], z[1000];
	float vx[1000], vy[1000], vz[1000];
};
```

---

## 9. Organização de Arquivos

### 9.1 Estrutura de Diretórios

```
Roda um ls e le documentação ai kk
```

### 9.2 Um Conceito por Arquivo

```c
/* bhs_vec3.h - APENAS operações de Vec3 */
/* bhs_mat4.h - APENAS operações de Mat4 */
/* bhs_geodesica.h - APENAS geodésicas */

/* NÃO: bhs_matematica.h com 5000 linhas de tudo */
```

### 9.3 Headers Públicos vs Privados

```
include/          # Headers públicos (API externa)
├── bhs_types.h
├── bhs_mesh.h
└── bhs_sim.h

src/modulo/       # Headers privados (implementação)
├── modulo.c
├── modulo_interno.h
└── modulo_helpers.c
```

---

## 10. Invariantes e Contratos

### 10.1 O Que São Invariantes

Invariantes são **condições que DEVEM ser verdadeiras** em pontos específicos do código. Documente-os explicitamente.

### 10.2 Exemplos

```c
/**
 * struct bhs_mesh - Malha do espaço-tempo
 *
 * Invariantes:
 * - vertices != NULL após inicialização
 * - n_vertices > 0 após inicialização
 * - indices são válidos: todos < n_vertices
 * - A malha é topologicamente fechada (sem buracos)
 */
struct bhs_mesh {
	struct bhs_vec3 *vertices;
	uint32_t n_vertices;
	uint32_t *indices;
	uint32_t n_indices;
};

/**
 * bhs_mesh_validar - Verifica invariantes da malha
 *
 * Retorna true se todos os invariantes são satisfeitos.
 * Use em builds de debug e testes.
 */
bool bhs_mesh_validar(const struct bhs_mesh *m);
```

### 10.3 Asserts de Debug

```c
#ifdef BHS_DEBUG
#define BHS_ASSERT(cond) \
	do { \
		if (!(cond)) { \
			fprintf(stderr, "ASSERT FALHOU: %s (%s:%d)\n", \
				#cond, __FILE__, __LINE__); \
			abort(); \
		} \
	} while (0)
#else
#define BHS_ASSERT(cond) ((void)0)
#endif

void funcao_critica(struct bhs_body *b)
{
	BHS_ASSERT(b != NULL);
	BHS_ASSERT(b->mass > 0);
	/* ... */
}
```

---

## 11. Checklist Antes de Commitar

Antes de qualquer commit, verifique:

### Código
- [ ] Compila sem warnings com `-Wall -Wextra -Werror`
- [ ] Sem memory leaks (rode Valgrind)
- [ ] Sem undefined behavior (rode com sanitizers)
- [ ] Testes passam

### Estilo
- [ ] Tabs de 8 espaços (ou tab literal)
- [ ] Linhas ≤ 80 caracteres (ideal), ≤ 100 (máximo absoluto)
- [ ] Nomenclatura consistente com o projeto
- [ ] Comentários em português

### Documentação
- [ ] Funções públicas documentadas
- [ ] Invariantes declarados
- [ ] TODOs têm contexto suficiente

### Performance
- [ ] Sem alocações desnecessárias em loops
- [ ] Complexidade algorítmica aceitável
- [ ] Sem cálculos repetidos óbvios

---

## Palavras Finais

> "Qualquer idiota consegue escrever código que um computador entende.
> Bons programadores escrevem código que humanos entendem."
> — Martin Fowler

Agora vai lá e escreve código bonito. E se escrever código feio, pelo menos seja honesto nos comentários sobre isso.

```c
/*
 * Desculpa pelo código abaixo.
 * Eu tentei fazer melhor, juro.
 * (mentira, quero q se foda kkkkkkkkkk)
 * vai toma no cu kkkkkkkkkk
 */
```
