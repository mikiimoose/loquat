#ifndef AI_COMMON_H
#define AI_COMMON_H

// Structure to hold response data
struct ResponseData {
    char *data;
    size_t size;
};

// Structure to hold parsed response fields
typedef struct {
    char *model;
    char *response;
    char *done;
    double total_duration;
    double load_duration;
    double prompt_eval_count;
    double prompt_eval_duration;
    double eval_count;
    double eval_duration;
} OllamaResponse;

char* ask_ollama(const char* question);

char* ask_chatgpt(char* question);

#endif
