# SOURCE CODE (SRC)

Raiz do código fonte do Black Hole Simulator. Arquitetura em camadas estritas:
1. `core`: Matemática Pura e Física Teórica (sem dependências).
2. `engine`: Modelos Físicos Aplicados (Disco, Geodésicas, Corpos).
3. `sim`: Loop de Simulação e Orquestração de Cena.
4. `ux`: Interface Gráfica, Renderização e Plataforma.
Dependências fluem apenas para baixo (UX -> Sim -> Engine -> Core).
