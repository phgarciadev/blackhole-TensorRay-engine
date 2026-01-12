# UX LIB

Framework de UI "Immediate Mode" Unificado.
Esta é a "Game Engine" da interface.
Agrega o sistema de Janelas (`window`), Renderização 2D (`render`), Widgets (`widgets.c`) e Layouts.
Fornce a API pública `lib.h` para criar aplicações: `bhs_ui_button`, `bhs_ui_begin_frame`, etc.
Oculta toda a complexidade de Vulkan/Wayland do programador da aplicação.
