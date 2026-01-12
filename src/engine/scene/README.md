# SIM SCENE

Orquestrador de alto nível da simulação.
Gerencia o ciclo de vida da "Cena", que é o container de todas as entidades (Corpos Celestes) e do ambiente (Malha Espaço-Tempo).
Responsável por:
- Inicializar o estado do universo (setup default).
- Executar o loop de atualização física (CPU-side logic).
- Expor os dados para a camada de visualização (UX).
Basicamente, o "Game World".
