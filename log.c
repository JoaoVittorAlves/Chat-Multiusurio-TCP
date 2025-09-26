#include "log.h"

void logger_init(Logger* logger) {
    pthread_mutex_init(&logger->mutex, NULL);
}

void logger_log(Logger* logger, const char* fmt, ...) {
    pthread_mutex_lock(&logger->mutex);

    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    printf("\n");
    va_end(args);

    pthread_mutex_unlock(&logger->mutex);
}
