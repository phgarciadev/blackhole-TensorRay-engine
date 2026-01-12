# UX LIB WINDOW

Gerenciamento de Janelas e Eventos.
Abstrai o loop de eventos principal (`poll_events`), redimensionamento e controle de cursor.
Coordena a comunicação entre a camada de Plataforma (OS) e o Contexto UI.
Garante que a janela esteja pronta para apresentação antes de renderizar.
