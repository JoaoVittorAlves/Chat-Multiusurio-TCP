# Chat Multiusuário TCP com Logging Thread-Safe

## Descrição
Este projeto implementa um **chat multiusuário TCP** em C, com logging thread-safe integrado. Inclui cliente e servidor, ambos usando uma biblioteca de logging baseada em mutex (`log.h`/`log.c`). Há também scripts para simular múltiplos clientes e testar concorrência.

---

## Estrutura do Projeto

- `server.c` — Servidor TCP multiusuário (aceita clientes, retransmite mensagens, faz logging)
- `client.c` — Cliente TCP (envia e recebe mensagens, faz logging)
- `log.h` / `log.c` — Biblioteca de logging thread-safe
- `test_clients.sh` — Script para simular múltiplos clientes automaticamente
- `test_logging.c` — Teste independente do logger (concorrência)
- `Makefile` — Compilação dos binários

---

## Funcionalidades

- **Servidor:** Aceita múltiplos clientes, retransmite mensagens entre eles, faz logging thread-safe de eventos.
- **Cliente:** Conecta ao servidor, envia e recebe mensagens, faz logging thread-safe de eventos.
- **Logging:** Todas as mensagens e eventos importantes são registrados de forma thread-safe.
- **Script de Teste:** `test_clients.sh` executa o servidor e simula múltiplos clientes enviando mensagens automaticamente.

---

## Como Compilar

```bash
make
```

---

## Como Executar

### Servidor
```bash
./server
```

### Cliente
```bash
./client
```

### Teste automatizado com múltiplos clientes
```bash
chmod +x test_clients.sh
./test_clients.sh
```

---

## Teste do Logger (opcional)

```bash
gcc -o test_logging test_logging.c log.c -lpthread
./test_logging
```

---

## Modos de Execução

O mesmo binário de servidor e cliente suporta diferentes modos, selecionados por argumento de linha de comando:

### 1. Chat Multiusuário (padrão)
- Servidor:
  ```bash
  ./server chat
  # ou simplesmente ./server
  ```
- Cliente:
  ```bash
  ./client chat
  # ou simplesmente ./client
  ```

### 2. Agendador (envio de job)
- Servidor:
  ```bash
  ./server agendador
  ```
- Cliente:
  ```bash
  ./client agendador
  ```
  O cliente pedirá para digitar um job, enviará ao servidor e mostrará a resposta de reconhecimento.

### 3. Web (GET HTTP)
- Servidor:
  ```bash
  ./server web
  ```
- Teste com navegador ou curl:
  ```bash
  curl http://localhost:8081/
  ```
  O servidor responde a requisições GET com uma mensagem simples.

---

## Observações
- O logger thread-safe está integrado em todos os modos.
- O script `test_clients.sh` só testa o modo chat.
- Para cada modo, use a porta padrão do exemplo ou ajuste conforme necessário no código.
- O cliente não possui modo web, pois o teste HTTP é feito via navegador ou curl.
