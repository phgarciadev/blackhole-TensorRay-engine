# UX PLATFORM

Abstração do Sistema Operacional.
Gerencia a criação de janelas, recepção de inputs (teclado/mouse) e interação com o Window Manager.
Define a interface `bhs_platform_window` que deve ser implementada por cada backend (Wayland, X11, Win32, Cocoa).
Isola o resto da engine de detalhes de sistema.
