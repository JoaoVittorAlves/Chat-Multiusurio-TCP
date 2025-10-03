#!/bin/bash

# Número de clientes para testar
NUM_CLIENTS=3

# Mensagem de teste para cada cliente
declare -a MENSAGENS=("Hello do cliente 1" "Hello do cliente 2" "Hello do cliente 3")

# Inicia servidor em background
./server &
SERVER_PID=$!
echo "Servidor iniciado (PID=$SERVER_PID)"
sleep 1  # espera servidor subir

# Função para iniciar um cliente enviando uma mensagem
run_client() {
    local idx=$1
    local msg="${MENSAGENS[$idx]}"
    # Envia a mensagem e espera 1 segundo antes de encerrar
    echo "$msg" | ./client &
}

# Inicia múltiplos clientes
for i in $(seq 0 $((NUM_CLIENTS-1))); do
    run_client $i
done

# Espera os clientes se comunicarem
sleep 5

# Mata o servidor
kill $SERVER_PID
echo "Servidor encerrado"
