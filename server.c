#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include "log.h"

// Definições
#define PORT_CHAT 8080
#define PORT_WEB 8081
#define PORT_AGENDADOR 8082
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

// Estrutura monitor para clientes
typedef struct {
    int socket[MAX_CLIENTS];
    int count;
    pthread_mutex_t mutex;
} ThreadSafeClientsList;

ThreadSafeClientsList clients_list = {.count = 0, .mutex = PTHREAD_MUTEX_INITIALIZER};

// Semáforo
sem_t sem_clients;

// Variáveis globais
Logger logger;
int running = 1;

// ===================== Funções de controle =====================
void client_list_add(int sock) {
    pthread_mutex_lock(&clients_list.mutex);
    if (clients_list.count < MAX_CLIENTS) {
        clients_list.socket[clients_list.count++] = sock;
    }
    pthread_mutex_unlock(&clients_list.mutex);
}

void client_list_remove(int sock) {
    pthread_mutex_lock(&clients_list.mutex);
    for (int i = 0; i < clients_list.count; i++) {
        if (clients_list.socket[i] == sock) {
            clients_list.socket[i] = clients_list.socket[clients_list.count - 1];
            clients_list.count--;
            break;
        }
    }
    pthread_mutex_unlock(&clients_list.mutex);
}

// Envia mensagem para todos os clientes
void broadcast(const char* message, int sender_sock) {
    pthread_mutex_lock(&clients_list.mutex);
    for (int i = 0; i < clients_list.count; i++) {
        int sock = clients_list.socket[i];
        if (sock != sender_sock) {
            int sent = send(sock, message, strlen(message), 0);
            if (sent <= 0) {
                logger_log(&logger, "Falha ao enviar mensagem para socket %d (ignorado temporariamente)", sock);
                
            }
        }
    }
    pthread_mutex_unlock(&clients_list.mutex);
}

// Lida com cliente (mais robusto)
void* client_handler(void* arg) {
    int client_sock = *(int*)arg;
    free(arg);
    char buffer[BUFFER_SIZE];
    char bye[] = "Conexão encerrada.\n";

    while (1) {
        int bytes = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes < 0) {
            logger_log(&logger, "Erro de recv no socket %d", client_sock);
            continue; // tenta novamente
        } else if (bytes == 0) {
            logger_log(&logger, "Cliente desconectou (socket %d)", client_sock);
            break; // saída do loop, vai fechar
        }

        buffer[bytes] = '\0';
        logger_log(&logger, "Mensagem recebida do socket %d: %s", client_sock, buffer);

        if (strncmp(buffer, "SAIR", 4) == 0) {
            send(client_sock, bye, strlen(bye), 0);
            break;
        }

        broadcast(buffer, client_sock);
    }

    close(client_sock);
    client_list_remove(client_sock);
    sem_post(&sem_clients);
    logger_log(&logger, "Conexão encerrada com socket %d", client_sock);
    return NULL;
}


// ===================== Servidores =====================
void run_chat_server() {
    int server_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;

    printf("=== Servidor TCP Multiusuário (CHAT) ===\n");

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT_CHAT);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erro ao fazer bind");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 5) < 0) {
        perror("Erro ao fazer listen");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    logger_log(&logger, "Servidor de chat ouvindo na porta %d", PORT_CHAT);

    while (running) {
        client_addr_len = sizeof(client_addr);
        sem_wait(&sem_clients);

        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("Erro ao aceitar conexão");
            sem_post(&sem_clients);
            continue;
        }

        client_list_add(client_socket);

        // Exibe IP e porta do cliente
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        printf("Nova conexão: %s:%d (socket %d)\n", client_ip, ntohs(client_addr.sin_port), client_socket);

        pthread_t tid;
        int* pclient = malloc(sizeof(int));
        if (!pclient) {
            perror("malloc falhou");
            close(client_socket);
            sem_post(&sem_clients);
            continue;
        }
        *pclient = client_socket;
        pthread_create(&tid, NULL, client_handler, pclient);
        pthread_detach(tid);
    }

    close(server_socket);
}

// ===================== Servidor WEB =====================
void run_web_server() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[2048];

    printf("=== Servidor HTTP (WEB) ===\n");

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
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

    while (running) {
        client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (client_fd < 0) continue;

        memset(buffer, 0, sizeof(buffer));
        int valread = read(client_fd, buffer, sizeof(buffer) - 1);
        if (valread <= 0) {
            close(client_fd);
            continue;
        }
        buffer[valread] = '\0';

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

// ===================== Servidor AGENDADOR =====================
void run_agendador_server() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1024];

    printf("=== Servidor Agendador ===\n");

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
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

    while (running) {
        client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (client_fd < 0) continue;

        memset(buffer, 0, sizeof(buffer));
        int valread = read(client_fd, buffer, sizeof(buffer) - 1);
        if (valread <= 0) {
            close(client_fd);
            continue;
        }
        buffer[valread] = '\0';

        logger_log(&logger, "Job recebido: %s", buffer);

        char response[1024];
        snprintf(response, sizeof(response), "Job recebido e reconhecido: %s\n", buffer);
        send(client_fd, response, strlen(response), 0);

        close(client_fd);
    }

    close(server_fd);
}

// ===================== Encerramento limpo =====================
void handle_sigint(int sig) {
    (void)sig;
    running = 0;
    printf("\nEncerrando servidores...\n");
    pthread_mutex_destroy(&clients_list.mutex);
    sem_destroy(&sem_clients);
    logger_close(&logger);
    exit(0);
}

// ===================== Função principal =====================
int main(int argc, char* argv[]) {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, handle_sigint);

    logger_init(&logger);
    sem_init(&sem_clients, 0, MAX_CLIENTS);

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

    sem_destroy(&sem_clients);
    logger_close(&logger);
    return 0;
}
