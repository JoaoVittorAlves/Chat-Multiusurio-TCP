#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "log.h"

// Definições de constantes
#define PORT_CHAT 8080
#define PORT_WEB 8081
#define PORT_AGENDADOR 8082
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

// Protótipos das funções de serviço
void broadcast(const char* message, int sender_sock);
void* client_handler(void* arg);
void run_chat_server();
void run_web_server();
void run_agendador_server();

// Variáveis globais
Logger logger;
int clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
* Função principal do servidor
*/
int main(int argc, char* argv[]) {
    logger_init(&logger);
    if (argc < 2 || strcmp(argv[1], "chat") == 0) {
        run_chat_server();
    } else if (strcmp(argv[1], "web") == 0) {
        run_web_server();
    } else if (strcmp(argv[1], "agendador") == 0) {
        run_agendador_server();
    } else {
        printf("Modo inválido. Use: %s [chat|web|agendador]\n", argv[0]);
        return 1;
    }
    return 0;
}

// ================= CHAT =================
void run_chat_server() {
    int server_socket; // Socket do servidor (ouvinte)
    int client_socket; // Socket para comunicação com cliente
    struct sockaddr_in server_addr; // Endereço do servidor
    struct sockaddr_in client_addr; // Endereço do cliente
    socklen_t client_addr_len;

    printf("=== Servidor TCP Multiusuário (CHAT) ===\n");

    // Criando socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }
    printf("✓ Socket criado com sucesso (fd: %d)\n", server_socket);

    // Configurando endereço do servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT_CHAT);

    // Associa socket ao endereço (bind)
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erro ao fazer bind");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    printf("Bind realizado na porta %d\n", PORT_CHAT);

    // Marca socket como passivo (listen)
    if (listen(server_socket, 5) < 0) {
        perror("Erro ao fazer listen");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    printf("Servidor ouvindo na porta %d...\n", PORT_CHAT);
    printf("Aguardando conexões de clientes...\n\n");

    // LOOP PRINCIPAL: Aceita e processa conexões de clientes
    while (1) {
        client_addr_len = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("Erro ao aceitar conexão");
            continue;
        }

        // Converte IP do cliente para formato legível
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("========================================\n");
        printf("Nova conexão aceita!\n");
        printf("Cliente: %s:%d\n", client_ip, ntohs(client_addr.sin_port));
        printf("Socket do cliente: %d\n", client_socket);
        printf("========================================\n");

        // Envia mensagem de boas-vindas
        char welcome[] = "=== Bem-vindo ao chat TCP! ===\nDigite mensagens e elas serão enviadas para todos.\nDigite 'SAIR' para encerrar.\n";
        send(client_socket, welcome, strlen(welcome), 0);

        // Adiciona à lista de clientes
        pthread_mutex_lock(&clients_mutex);
        if (client_count < MAX_CLIENTS) {
            clients[client_count++] = client_socket;
        } else {
            logger_log(&logger, "Máximo de clientes atingido, desconectando socket %d", client_socket);
            close(client_socket);
            pthread_mutex_unlock(&clients_mutex);
            continue;
        }
        pthread_mutex_unlock(&clients_mutex);

        int* pclient = malloc(sizeof(int));
        *pclient = client_socket;

        pthread_t tid;
        pthread_create(&tid, NULL, client_handler, pclient);
        pthread_detach(tid);
    }

    close(server_socket);
}

// Envia mensagem para todos os clientes (broadcast)
void broadcast(const char* message, int sender_sock) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        int sock = clients[i];
        if (sock == sender_sock) continue; // não envia de volta para quem mandou
        send(sock, message, strlen(message), 0);
    }
    pthread_mutex_unlock(&clients_mutex);
}

// Função que processa as requisições do cliente
void* client_handler(void* arg) {
    int client_sock = *(int*)arg;
    free(arg);
    char buffer[BUFFER_SIZE];

    while (1) {
        int bytes = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            logger_log(&logger, "Cliente desconectado (socket %d)", client_sock);
            close(client_sock);

            // Remove da lista
            pthread_mutex_lock(&clients_mutex);
            for (int i = 0; i < client_count; i++) {
                if (clients[i] == client_sock) {
                    clients[i] = clients[client_count - 1];
                    client_count--;
                    break;
                }
            }
            pthread_mutex_unlock(&clients_mutex);
            return NULL;
        }

        buffer[bytes] = '\0';
        logger_log(&logger, "Mensagem recebida: %s", buffer);

        // Envia para todos os outros clientes
        broadcast(buffer, client_sock);
    }
    return NULL;
}

// ================= WEB =================
void run_web_server() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[2048];

    printf("=== Servidor HTTP (WEB) ===\n");

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT_WEB);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    logger_log(&logger, "Servidor HTTP ouvindo na porta %d", PORT_WEB);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        memset(buffer, 0, sizeof(buffer));
        int valread = read(client_fd, buffer, sizeof(buffer) - 1);
        logger_log(&logger, "Requisição recebida:\n%s", buffer);

        if (strncmp(buffer, "GET ", 4) == 0) {
            char response[] = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nServidor HTTP C: GET recebido com sucesso!\n";
            send(client_fd, response, strlen(response), 0);
        } else {
            char response[] = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\nRequisição não suportada.\n";
            send(client_fd, response, strlen(response), 0);
        }

        close(client_fd);
    }

    close(server_fd);
}

// ================= AGENDADOR =================
void run_agendador_server() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1024];

    printf("=== Servidor Agendador ===\n");

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT_AGENDADOR);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    logger_log(&logger, "Servidor Agendador ouvindo na porta %d", PORT_AGENDADOR);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        memset(buffer, 0, sizeof(buffer));
        int valread = read(client_fd, buffer, sizeof(buffer) - 1);
        logger_log(&logger, "Job recebido: %s", buffer);

        char response[1024];
        snprintf(response, sizeof(response), "Job recebido e reconhecido: %s\n", buffer);
        send(client_fd, response, strlen(response), 0);

        close(client_fd);
    }

    close(server_fd);
}
