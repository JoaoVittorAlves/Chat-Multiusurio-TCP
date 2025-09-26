# Projeto: Logging Thread-Safe em C

## Descrição
Esta etapa do projeto  tem como objetivo implementar uma **biblioteca de logging thread-safe** em C, testada por um CLI que cria múltiplas threads gravando logs simultaneamente. O objetivo é demonstrar domínio de **programação concorrente**, uso de **mutex** e encapsulamento da sincronização em uma API clara.

---

## Estrutura do Projeto

- `log.h` — Cabeçalho da biblioteca de logging
- `log.c` — Implementação da biblioteca (mutex + log)
- `main.c` — CLI de teste com múltiplas threads
- `Makefile` — Build do projeto

---

## Funcionalidades

- Logging thread-safe usando `pthread_mutex_t`
- API clara: `logger_init()` e `logger_log()`
- CLI de teste: cria várias threads que gravam mensagens simultaneamente
- Output seguro no terminal, sem sobreposição de mensagens

---

## Como Compilar

```bash
make
```

## Como Executar

```bash
./main
```

O programa cria 5 threads (configurável) que gravam mensagens no console. Ao final, o teste de logging thread-safe é concluído.
