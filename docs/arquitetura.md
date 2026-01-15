# Arquitetura do Projeto - Layered Architecture

Este projeto usa uma **Arquitetura em Camadas** estritamente desacoplada.
As camadas ficam na **raiz do projeto**, não dentro de src/.
Se você mexer na camada errada, a culpa é sua. Tá avisado.

---

## Diagrama de Dependências

```
┌─────────────────────────────────────────────────────────┐
│                    SRC (src/)                           │
│              Entry point, UI específica                 │
│                  → blackhole_sim                        │
└──────────────────────────┬──────────────────────────────┘
                           │ depends on
           ┌───────────────┴───────────────┐
           │                               │
           ▼                               ▼
┌──────────────────────┐     ┌──────────────────────────┐
│   ENGINE (engine/)   │     │  FRAMEWORK (framework/)  │
│   libbhs_engine.a    │     │   libbhs_framework.a     │
│                      │     │                          │
│  • ECS               │     │  • Platform (Wayland,    │
│  • Physics           │     │    X11, Cocoa, Win32)    │
│  • Geodesics         │     │  • RHI (Vulkan, Metal,   │
│  • Bodies            │────▶│    OpenGL, DirectX)      │
│  • Scene             │     │  • UI Framework          │
└───────────┬──────────┘     └────────────┬─────────────┘
            │                              │
            │         depends on           │
            └──────────────┬───────────────┘
                           │
                           ▼
           ┌───────────────────────────────┐
           │       MATH (math/)            │
           │       libbhs_math.a           │
           │                               │
           │  • vec4.h/c (vetores 4D)      │
           │  • tensor/ (métricas)         │
           │  • spacetime/ (Kerr, Schwarz) │
           │  • core.h (unified include)   │
           │                               │
           │  ZERO dependências externas   │
           │  (só <math.h>, <stdint.h>)    │
           └───────────────────────────────┘
```

---

## Estrutura de Diretórios

```
blackholegravity/
├── math/                       # Camada 1: Matemática pura
│   ├── Makefile               → libbhs_math.a
│   ├── bhs_math.h             # Tipos base (real_t)
│   ├── vec4.h/c               # Vetores 4D
│   ├── core.h                 # Include unificado
│   ├── tensor/                # Tensores métricos
│   │   └── tensor.h/c
│   └── spacetime/             # Métricas de espaço-tempo
│       ├── kerr.h/c
│       └── schwarzschild.h/c
│
├── framework/                  # Camada 2: HAL + RHI + UI
│   ├── Makefile               → libbhs_framework.a
│   ├── platform/              # Abstração de OS
│   │   ├── platform.h         # API pública
│   │   ├── wayland/           # Backend Linux moderno
│   │   ├── x11/               # Backend Linux legado
│   │   ├── cocoa/             # Backend macOS
│   │   └── win32/             # Backend Windows
│   ├── rhi/                   # Render Hardware Interface
│   │   ├── renderer.h         # API pública
│   │   ├── vulkan/            # Backend Vulkan
│   │   ├── metal/             # Backend Metal
│   │   ├── opengl/            # Backend OpenGL
│   │   └── dx/                # Backend DirectX
│   └── ui/                    # UI Framework
│       ├── lib.h              # API pública
│       ├── context.c          # Gerenciamento de contexto
│       ├── widgets.c          # Botões, sliders, etc
│       ├── render/            # Renderização 2D
│       └── window/            # Gerenciamento de janela
│
├── engine/                     # Camada 3: Simulação
│   ├── Makefile               → libbhs_engine.a
│   ├── engine.h               # API pública
│   ├── ecs/                   # Entity Component System
│   ├── assets/                # Carregamento de recursos
│   ├── body/                  # Corpos celestes
│   ├── geodesic/              # Trajetórias de luz
│   ├── scene/                 # Gerenciamento de cena
│   ├── systems/               # Sistemas ECS
│   └── planets/               # Dados de planetas
│
├── src/                        # Camada 4: Aplicação (Entry Point)
│   ├── Makefile               → blackhole_sim
│   ├── main.c                 # Entry point
│   ├── ui/                    # UI específica do app
│   │   ├── camera/            # Controle de câmera
│   │   ├── render/            # Renderização espacial
│   │   └── screens/           # Telas (HUD, etc)
│   └── debug/                 # Ferramentas de debug
│
├── build/                      # Artefatos de build
│   ├── math/libbhs_math.a
│   ├── framework/libbhs_framework.a
│   ├── engine/libbhs_engine.a
│   └── blackhole_sim
│
└── Makefile                    # Build system principal
```

---

## Regras de Ouro

### 1. Hierarquia de Dependências

```
src/ → engine/ → framework/ → math/
src/ → framework/ → math/
src/ → math/
engine/ → framework/ → math/
engine/ → math/
framework/ → math/
```

**PROIBIDO**: Dependências circulares ou inversas.

### 2. Cada Camada Gera uma Biblioteca

| Camada | Output | Responsabilidade |
|--------|--------|------------------|
| math/ | `libbhs_math.a` | Computação pura (ZERO deps externas) |
| framework/ | `libbhs_framework.a` | Hardware abstraction (não sabe de física) |
| engine/ | `libbhs_engine.a` | Lógica de simulação |
| src/ | `blackhole_sim` | Glue code, entry point |

### 3. Includes Padronizados

Use includes relativos à raiz do projeto:

```c
/* CORRETO */
#include "math/vec4.h"
#include "framework/rhi/renderer.h"
#include "engine/scene/scene.h"
#include "src/ui/camera/camera.h"

/* ERRADO (caminhos relativos feios) */
#include "../../math/vec4.h"
#include "../framework/rhi/renderer.h"
```

---

## Build

```bash
# Build completo
make all

# Build por camada
make math       # Só matemática
make framework  # Só framework (depende de math)
make engine     # Só engine (depende de math, framework)
make src        # Só app (depende de tudo)

# Limpar
make clean

# Info
make info
```

---

## Constraints por Camada

### math/ - ZERO deps externas
```c
/* math/ SÓ pode incluir: */
#include <math.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* PROIBIDO em math/: */
#include "framework/..."  // NÃO!
#include "engine/..."     // NÃO!
#include <vulkan.h>       // NÃO!
```

### framework/ - Não sabe de física
```c
/* framework/ pode incluir: */
#include "math/..."

/* framework/ NÃO sabe o que é: */
// Black hole, planet, geodesic, spacetime, Kerr metric...
// Só sabe: triangles, buffers, windows, events
```

### engine/ - O cérebro
```c
/* engine/ pode incluir: */
#include "math/..."
#include "framework/..."

/* engine/ contém toda a lógica de: */
// ECS, physics, bodies, geodesics, scene management
```

### src/ - Glue code
```c
/* src/ pode incluir TUDO: */
#include "math/..."
#include "framework/..."
#include "engine/..."
#include "src/..."  // UI específica

/* src/ é responsável por: */
// main(), inicialização, loop principal, UI específica
```

---

## Para Contribuidores

Antes de contribuir, leia:

1. Este arquivo (`docs/arquitetura.md`)
2. `.gemini/escrevendo-codigo.md` - Regras de estilo
3. O `lib.h` ou header principal do módulo que vai mexer

**Se você criar dependência circular, o build quebra. E a culpa é sua.**
