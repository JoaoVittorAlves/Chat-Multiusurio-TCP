#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "log.h"

#define THREADS 5

Logger logger;

void* thread_func(void* arg) {
    int id = *(int*)arg;
    for (int i = 0; i < 5; i++) {
        logger_log(&logger, "Thread %d, mensagem %d", id, i);
        usleep(100000); // 0.1s
    }
    return NULL;
}

int main() {
    pthread_t tids[THREADS];
    int ids[THREADS];

    logger_init(&logger);

    // Cria múltiplas threads simulando concorrência
    for (int i = 0; i < THREADS; i++) {
        ids[i] = i+1;
        pthread_create(&tids[i], NULL, thread_func, &ids[i]);
    }

    // Espera todas terminarem
    for (int i = 0; i < THREADS; i++) {
        pthread_join(tids[i], NULL);
    }

    printf("Teste de logging thread-safe concluído!\n");
    return 0;
}
