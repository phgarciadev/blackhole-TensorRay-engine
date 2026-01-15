# SRC SIM

Camada de Simulação. Aqui o sistema ganha vida.
Diferente da Engine (que define as regras), a Sim define o "estado" e a "execução".
Contém o driver de física (`physics.c`) para despachar trabalho para a GPU e o gerenciador de cena (`scene`) para manter a coerência do mundo.
É a ponte entre a lógica pura e a aplicação final.
