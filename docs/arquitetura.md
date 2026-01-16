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
│   ENGINE (engine/)   │     │  gui-framework (gui-framework/)  │
│   libbhs_engine.a    │     │   libbhs_gui-framework.a     │
│                      │     │                          │
│  • ECS               │     │  • Platform (Wayland,    │
│  • Physics           │     │    X11, Cocoa, Win32)    │
│  • Geodesics         │     │  • RHI (Vulkan, Metal,   │
│  • Bodies            │────▶│    OpenGL, DirectX)      │
│  • Scene             │     │  • UI gui-framework          │
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
│   ├── CMakeLists.txt         → libbhs_math.a
│   ├── bhs_math.h             # Tipos base (real_t)
│   ├── vec4.h/c               # Vetores 4D
│   ├── core.h                 # Include unificado
│   ├── tensor/                # Tensores métricos
│   │   └── tensor.h/c
│   └── spacetime/             # Métricas de espaço-tempo
│       ├── kerr.h/c
│       └── schwarzschild.h/c
│
├── gui-framework/                  # Camada 2: HAL + RHI + UI
│   ├── CMakeLists.txt         → libbhs_gui-framework.a
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
│   └── ui/                    # UI gui-framework
│       ├── lib.h              # API pública
│       ├── context.c          # Gerenciamento de contexto
│       ├── widgets.c          # Botões, sliders, etc
│       ├── render/            # Renderização 2D
│       └── window/            # Gerenciamento de janela
│
├── engine/                     # Camada 3: Simulação
│   ├── CMakeLists.txt         → libbhs_engine.a
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
│   ├── CMakeLists.txt         → blackhole_sim
│   ├── main.c                 # Entry point
│   ├── ui/                    # UI específica do app
│   │   ├── camera/            # Controle de câmera
│   │   ├── render/            # Renderização espacial
│   │   └── screens/           # Telas (HUD, etc)
│   └── debug/                 # Ferramentas de debug
│
├── build/                      # Artefatos de build (CMake)
│   ├── lib/libbhs_math.a
│   ├── lib/libbhs_gui-framework.a
│   ├── lib/libbhs_engine.a
│   └── bin/blackhole_sim
│
└── CMakeLists.txt              # Configuração principal do CMake
```

---

## Regras de Ouro

### 1. Hierarquia de Dependências

```
src/ → engine/ → gui-framework/ → math/
src/ → gui-framework/ → math/
src/ → math/
engine/ → gui-framework/ → math/
engine/ → math/
gui-framework/ → math/
```

**PROIBIDO**: Dependências circulares ou inversas.

### 2. Cada Camada Gera uma Biblioteca

| Camada | Output | Responsabilidade |
|--------|--------|------------------|
| math/ | `libbhs_math.a` | Computação pura (ZERO deps externas) |
| gui-framework/ | `libbhs_gui-framework.a` | Hardware abstraction (não sabe de física) |
| engine/ | `libbhs_engine.a` | Lógica de simulação |
| src/ | `blackhole_sim` | Glue code, entry point |

### 3. Includes Padronizados

Use includes relativos à raiz do projeto:

```c
/* CORRETO */
#include "math/vec4.h"
#include "gui-framework/rhi/renderer.h"
#include "engine/scene/scene.h"
#include "src/ui/camera/camera.h"

/* ERRADO (caminhos relativos feios) */
#include "../../math/vec4.h"
#include "../gui-framework/rhi/renderer.h"
```

---

## Build System (CMake)

```bash
# Configurar (gera arquivos de build na pasta 'build')
# Requer CMake 3.16+
cmake -B build -S .

# Compilar tudo
cmake --build build

# Rodar Testes
cd build && ctest

# Opções de Configuração:
# -DBHS_ENABLE_TESTING=ON/OFF (Padrão: ON)
# -DBHS_ENABLE_SANITIZERS=ON/OFF (Padrão: OFF - Requer Clang/GCC recente)
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
#include "gui-framework/..."  // NÃO!
#include "engine/..."     // NÃO!
#include <vulkan.h>       // NÃO!
```

### gui-framework/ - Não sabe de física
```c
/* gui-framework/ pode incluir: */
#include "math/..."

/* gui-framework/ NÃO sabe o que é: */
// Black hole, planet, geodesic, spacetime, Kerr metric...
// Só sabe: triangles, buffers, windows, events
```

### engine/ - O cérebro
```c
/* engine/ pode incluir: */
#include "math/..."
#include "gui-framework/..."

/* engine/ contém toda a lógica de: */
// ECS, physics, bodies, geodesics, scene management
```

### src/ - Glue code
```c
/* src/ pode incluir TUDO: */
#include "math/..."
#include "gui-framework/..."
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
