#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "handling.h"
#include "logger.h"
#include <unistd.h>


static void* convert(void* arg) {
    char* filename = (char*) arg;
    if (filename == NULL) {
        log_message(LOG_ERR, "Filename is NULL");
        return NULL;
    }

    log_message(LOG_INFO, "Converting speech to text");

    char* text = NULL;
    if (text) {
        log_message(LOG_INFO, "%s", text);
        free(text);
    } else {
        log_message(LOG_ERR, "Empty voice ----------");
    }
    unlink(filename);
    free(filename);
    return NULL;
}


// Start a thread to convert speech to text

void start_speech_to_text(char* filename) {
    char* file = strdup(filename);

    pthread_t thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_create(&thread, &attr, convert, file)) {
        log_message(LOG_ERR, "Error creating thread");
        return;
    }

    pthread_attr_destroy(&attr);
}