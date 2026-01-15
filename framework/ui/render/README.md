# UX LIB RENDER

Motor de renderização 2D da UI.
Implementa o pipeline de desenho para primitivas básicas (Retângulos, Linhas, Texto).
Responsável por montar batches de vértices (`render2d.c`) e submetê-los ao backend gráfico.
Gerencia a alocação de buffers de vértices/índices e o setup de pipelines (blend states).
