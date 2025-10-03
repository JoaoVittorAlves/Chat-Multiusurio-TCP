#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "log.h"

#define SERVER_IP "127.0.0.1" // IP do servidor (localhost)
#define PORT_CHAT 8080
#define PORT_AGENDADOR 8082
#define BUFFER_SIZE 1024 // Tamanho do buffer

void run_chat_client();
void run_agendador_client();

int main(int argc, char* argv[]) {
    if (argc < 2 || strcmp(argv[1], "chat") == 0) {
        run_chat_client();
    } else if (strcmp(argv[1], "agendador") == 0) {
        run_agendador_client();
    } else {
        printf("Modo inválido. Use: %s [chat|agendador]\n", argv[0]);
        return 1;
    }
    return 0;
}

void run_chat_client() {
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE];
    ssize_t bytes_received;
    Logger logger;

    logger_init(&logger);
    logger_log(&logger, "=== Cliente TCP - Modo Chat ===");

    // Criando socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        logger_log(&logger, "Erro ao criar socket");
        exit(EXIT_FAILURE);
    }
    logger_log(&logger, "✓ Socket criado");

    // Configurando endereço do servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT_CHAT);
    // Converte IP de string para formato binário
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        logger_log(&logger, "Endereço inválido");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // Conecta ao servidor
    logger_log(&logger, "Conectando a %s:%d...", SERVER_IP, PORT_CHAT);
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        logger_log(&logger, "Erro ao conectar");
        close(client_socket);
        exit(EXIT_FAILURE);
    }
    logger_log(&logger, "✓ Conectado ao servidor!\n");

    // Recebe e mostra menu
    memset(buffer, 0, BUFFER_SIZE);
    bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received > 0) {
        logger_log(&logger, "Menu recebido do servidor:");
        printf("%s", buffer);
        printf("\nDigite e envie uma mensagem para o chat!\n");
    }

    // Loop de interação com servidor
    while (1) {
        printf("Digite a mensagem: ");
        fflush(stdout);
        // Lê comando do usuário
        if (fgets(message, BUFFER_SIZE, stdin) == NULL) {
            break;
        }
        // Remove newline se existir
        message[strcspn(message, "\n")] = 0;
        // Verifica se usuário quer sair
        if (strlen(message) == 0) {
            continue;
        }
        // Envia comando para servidor
        if (send(client_socket, message, strlen(message), 0) < 0) {
            logger_log(&logger, "Erro ao enviar dados");
            break;
        }
        logger_log(&logger, "Mensagem enviada: %s", message);
        // Se comando foi SAIR, prepara para encerrar
        if (strcmp(message, "SAIR") == 0) {
            memset(buffer, 0, BUFFER_SIZE);
            recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
            logger_log(&logger, "Mensagem de encerramento recebida do servidor");
            printf("%s", buffer);
            break;
        }
        // Recebe resposta do servidor
        memset(buffer, 0, BUFFER_SIZE);
        bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                logger_log(&logger, "Servidor encerrou a conexão");
                printf("Servidor encerrou a conexão\n");
            } else {
                logger_log(&logger, "Erro ao receber dados");
                perror("Erro ao receber dados");
            }
            break;
        }
        // Mostra resposta do servidor
        logger_log(&logger, "Resposta recebida: %s", buffer);
        printf("\n%s\n", buffer);
        printf("Digite seu comando: ");
        fflush(stdout);
    }

    // Fecha o socket
    close(client_socket);
    logger_log(&logger, "Conexão encerrada.");
}

void run_agendador_client() {
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char job[BUFFER_SIZE];
    ssize_t bytes_received;
    Logger logger;

    logger_init(&logger);
    logger_log(&logger, "=== Cliente TCP - Modo Agendador ===");

    // Criando socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        logger_log(&logger, "Erro ao criar socket");
        exit(EXIT_FAILURE);
    }
    logger_log(&logger, "Socket criado");

    // Configurando endereço do servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT_AGENDADOR);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        logger_log(&logger, "Endereço inválido");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // Conecta ao servidor
    logger_log(&logger, "Conectando a %s:%d...", SERVER_IP, PORT_AGENDADOR);
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        logger_log(&logger, "Erro ao conectar");
        close(client_socket);
        exit(EXIT_FAILURE);
    }
    logger_log(&logger, "Conectado ao servidor!\n");

    // Prompt claro para o usuário
    printf("Digite o job para enviar ao agendador: ");
    fflush(stdout);
    if (fgets(job, BUFFER_SIZE, stdin) == NULL) {
        close(client_socket);
        return;
    }
    job[strcspn(job, "\n")] = 0;
    if (strlen(job) == 0) {
        printf("Job vazio, encerrando.\n");
        close(client_socket);
        return;
    }
    if (send(client_socket, job, strlen(job), 0) < 0) {
        logger_log(&logger, "Erro ao enviar dados");
        close(client_socket);
        return;
    }
    logger_log(&logger, "Job enviado: %s", job);
    memset(buffer, 0, BUFFER_SIZE);
    bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received > 0) {
        logger_log(&logger, "Resposta recebida: %s", buffer);
        printf("\nResposta do servidor: %s\n", buffer);
    } else {
        printf("\nNão foi possível receber resposta do servidor.\n");
    }
    close(client_socket);
    logger_log(&logger, "Conexão encerrada.");
}
