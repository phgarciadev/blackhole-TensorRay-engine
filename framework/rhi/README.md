# UX RENDERER

Abstração da GPU.
Define uma API unificada (`renderer.h`) para operações gráficas de baixo nível:
- Gerenciamento de Buffers e Texturas.
- Criação de Pipelines (Shaders, Blend States).
- Command Buffers e Render Passes.
Permite que a engine gráfica superior (`ux/lib`) funcione agnóstica de API (Vulkan/Metal/DX12).
