# 🚧 Engine Systems (ECS-based)

## Status: PREPARATÓRIO (Não Usado Atualmente)

Este diretório contém sistemas baseados em ECS (Entity Component System) que foram desenvolvidos para uma futura migração arquitetural, mas **NÃO são chamados atualmente** pela aplicação.

## Por Que Isso Existe?

O projeto tem dois sistemas de física paralelos:

| Sistema Ativo (Legado) | Sistema Preparatório (ECS) |
|------------------------|---------------------------|
| `scene.c` + `bhs_body[]` + `integrator.c` | `ecs.c` + `components.h` + estes sistemas |
| **Usado pela aplicação** | **Código morto** |

## Arquivos Neste Diretório

| Arquivo | Status | Descrição |
|---------|--------|-----------|
| `gravity_system.c/h` | 🔴 Não usado | Gravidade N-Body via ECS |
| `celestial_system.c/h` | 🔴 Não usado | Eventos de colisão celestial |
| `physics_system.c/h` | 🔴 Não usado | Integração de física ECS |

## Plano Futuro

Se/quando a migração para ECS for implementada:

1. `main.c` deve chamar `bhs_world_init()` de `engine/world/world.c`
2. Usar `bhs_gravity_system_nbody()` ao invés da física em `scene.c`
3. Conectar eventos via `bhs_celestial_system_init()`
4. Substituir `bhs_body[]` por entidades ECS

## Por Enquanto

Este código é mantido porque:
- Está bem escrito e documentado
- Representa a arquitetura desejada para o futuro
- Não afeta a compilação ou execução atual

**Não delete sem antes migrar para ECS ou ter certeza que não será usado.**
