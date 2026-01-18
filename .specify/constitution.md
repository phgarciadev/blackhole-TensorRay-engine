# A Constituição Suprema do Black Hole Simulator

> "Talk is cheap. Show me the code." — Linus Torvalds

Esta constituição define as leis irrevogáveis deste repositório. Violações resultarão em *public shaming* no code review ou, pior, execução lenta via `O(n^2)`.

## 1. Princípios Fundamentais (The Kernel Way)

1.  **Performance é Rei**: Se não roda em uma torradeira de 2010, é lixo. Priorize `O(1)`. `O(log n)` é tolerável. `O(n)` exige justificativa escrita em sangue.
2.  **C Puro & Rust no_std**: Nada de runtimes gordas, nada de garbage collectors pausando o universo. Nós controlamos a memória.
3.  **No Bullshit**: Código "esperto" é código burro. Código simples e direto vence. Se eu tiver que ler duas vezes para entender, está errado.
4.  **Invariantes ou Morte**: Toda função não trivial deve declarar suas pré-condições e pós-condições. Não assuma nada. Verifique tudo (em debug).
5.  **0% Bloat**: Dependências externas são proibidas até que se prove o contrário. `npm install` é um crime inafiançável (salvo ferramentas de build estritamente necessárias).

## 2. O Contrato de Desenvolvimento (Spec-Driven)

Ninguém escreve código "de cabeça". Nós seguimos o **Spec-Driven Development**.

1.  **Specify (Especifique)**: Antes de codar, descreva O QUE e POR QUE (`templates/spec.md`). Defina o contrato de entrada/saída.
2.  **Plan (Planeje)**: Defina COMO (`templates/plan.md`). Estruturas de dados, algoritmos, complexidade.
3.  **Execute (Implemente)**: Só abra o editor depois que o plano estiver sólido.
4.  **Verify (Verifique)**: Se não tem teste, não funciona. Se o teste é ruim, você é ruim.

## 3. Estilo e Comunicação

1.  **Português Brasileiro**: Documentação e comentários em PT-BR. Identificadores em Inglês.
2.  **Humor Ácido**: A vida é muito curta para code reviews corporativos chatos. Seja direto. Ironia é bem-vinda, passivo-agressividade é arte.
3.  **Emojis**: Proibidos em mensagens de commit sérias. Permitidos em comentários se for para tirar sarro de uma gambiarra temporária.

## 4. Definição de FEITO (Definition of Done)

Uma tarefa só está pronta quando:
- [ ] Compila sem warnings (`-Wall -Wextra -Werror` ou equivalente).
- [ ] Passa em todos os testes existentes.
- [ ] Novos testes cobrem os casos de borda e o "happy path".
- [ ] A complexidade ciclomática é menor que o ego do desenvolvedor médio.
- [ ] Não há alocações de memória dinâmica no "hot path" do loop de renderização/física.

---
*Assinado digitalmente pela autoridade suprema (a CI que vai quebrar seu build).*
