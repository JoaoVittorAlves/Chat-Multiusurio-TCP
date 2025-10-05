#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <pthread.h>
#include <stdarg.h>

typedef struct {
    pthread_mutex_t mutex;
} Logger;

void logger_init(Logger* logger);
void logger_log(Logger* logger, const char* fmt, ...);
void logger_close(Logger *logger);


#endif
