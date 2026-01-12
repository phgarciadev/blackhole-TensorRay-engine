# 🏗️ Arquitetura do Projeto

Leia isso antes de meter a mão no código. Sério. Não seja aquele dev que sai commitando sem entender onde tá pisando.

---

## Princípio Fundamental

> **Interface comum, implementações específicas.**

Tipo o kernel Linux com drivers. A aplicação conversa com uma API abstrata, e cada plataforma implementa do seu jeito usando APIs nativas. O código de alto nível nem sabe se tá rodando em Mac, Linux ou Windows.

---

## Estrutura UX (Interface Gráfica) 

```
Roda um ls ai kkkkkkk
```

---

## Como Funciona

### O `lib.h` do módulo (core.h, engine.h, etc) é o CHEFE

Define **O QUE** existe, não **COMO** funciona:

```c
/* platform/platform.h diz: */
int bhs_window_create(...);   /* "Preciso disso!" */
void bhs_window_destroy(...); /* "E disso também!" */
```

### O backend é o FUNCIONÁRIO

Implementa **COMO** fazer, usando APIs nativas:

```c
/* cocoa.mm responde: */
int bhs_window_create(...) {
    /* Usa NSWindow, faz macumba com AppKit... */
    return BHS_PLATFORM_OK;
}

/* win32.cpp responde: */
int bhs_window_create(...) {
    /* Usa CreateWindowEx, RegisterClass, a zona do Windows... */
    return BHS_PLATFORM_OK;
}
```

### Internamente, cada um faz o que quiser

Dentro do `.c`/`.cpp`/`.mm`, o backend pode ter:
- Structs auxiliares (ex: `BHSView`, `BHSWindowDelegate`)
- Funções helper privadas (ex: `bhs_cocoa_push_event()`)
- Estado global se necessário (mas evite, pelo amor)

**Nada disso é exposto publicamente.** A aplicação só enxerga o que `lib.h` define.

---

## Regras de Ouro

1. **Backends NÃO adicionam API pública** - Só implementam o que `lib.h` manda
2. **`lib.h` usa handles opacos** - `typedef struct impl *bhs_xxx_t`
3. **Erros são códigos negativos** - `0 = sucesso`, `< 0 = erro`
4. **Documentar invariantes** - O que DEVE ser verdade antes/depois

---

## No Build

Compila só o backend necessário:

```makefile
# Linux com X11
PLATFORM_SRC = src/ux/platform/x11/x11.c
RENDERER_SRC = src/ux/renderer/vulkan/vulkan.c

# macOS
PLATFORM_SRC = src/ux/platform/cocoa/cocoa.mm
RENDERER_SRC = src/ux/renderer/metal/metal.mm

# Windows
PLATFORM_SRC = src/ux/platform/win32/win32.cpp
RENDERER_SRC = src/ux/renderer/dx/directx.cpp
```

A aplicação nem percebe a diferença. Linka com a mesma API, roda em qualquer lugar.

---

## Analogia Final

Pensa num restaurante:
- **`lib.h`** = O cardápio (o que o cliente pode pedir)
- **Backend** = A cozinha (como o prato é feito)
- **Aplicação** = O cliente (só vê o cardápio, não a cozinha)

O cliente pede "bhs_window_create". A cozinha (cocoa.mm) faz usando NSWindow. Outra cozinha (win32.cpp) faz usando CreateWindowEx. O cliente recebe a janela e nem sabe como foi feita.

---

## Leitura Obrigatória

Antes de contribuir, leia nessa ordem:
1. Este arquivo (`arquitetura.md`) - Você está aqui
2. `escrevendo-codigo.md` - Regras de estilo e código
3. `lib.h` dos módulos que vai mexer - Entenda a interface

Agora sim, pode codar. Boa sorte, você vai precisar.
