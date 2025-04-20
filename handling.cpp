#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "handling.h"
#include "logger.h"
#include "ai_common.h"
#include "tts_espeak.h"
#include "network.h"
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
        char* ai_response;
        if (check_online_status() == 1) {
            ai_response = ask_chatgpt(text);
        } else {
            ai_response = ask_ollama(text);
        }
        if (ai_response) {
            espeak(ai_response);
        } else {
            char* errmessage = "Error: empty AI response";
            espeak(errmessage);
        }
        free(text);
        unlink(filename);
        free(filename);
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
