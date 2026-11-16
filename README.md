# Implementação 6 — Deadlock

Implementação do **Algoritmo do Banqueiro (Banker's Algorithm)** em linguagem C, desenvolvida para a disciplina de Infraestrutura de Software.

## Descrição

O programa gerencia a alocação de recursos entre clientes, verificando se as solicitações mantêm o sistema em um estado seguro, evitando situações de deadlock.

## Compilação

Para compilar o programa:

```bash
make
```

Para remover os arquivos compilados:

```bash
make clean
```

## Execução

O programa recebe, pela linha de comando, a quantidade disponível de cada tipo de recurso.

Exemplo:

```bash
./banker 10 5 7
```

## Arquivos

- `banker.c` — Implementação do algoritmo.
- `makefile` — Compilação e limpeza.
- `customer.txt` — Demanda máxima de recursos dos clientes.
- `commands.txt` — Comandos de requisição (`RQ`), liberação (`RL`) e exibição (`*`).
- `result.txt` — Resultado gerado pela execução.
- `evidencias.log` — Registro dos testes realizados.

## Saída

O programa gera o arquivo `result.txt`, contendo os resultados das solicitações, liberações e o estado dos recursos do sistema.