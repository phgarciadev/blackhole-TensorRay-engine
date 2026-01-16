# UX PLATFORM WAYLAND

Implementação da camada de plataforma para Linux via Wayland.
Usa `xdg-shell` para gerenciamento de janelas e protocolos de pointer-constraints para mouse look.
Responsável por negociar a superfície de desenho onde o Vulkan vai renderizar.
Código nativo usando `libwayland-client`.
