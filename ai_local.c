#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>
#include <stdbool.h>
#include "ai_common.h"

// Callback function to handle the response
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct ResponseData *resp = (struct ResponseData *)userp;

    char *ptr = realloc(resp->data, resp->size + realsize + 1);
    if (!ptr) {
        printf("Error: Out of memory\n");
        return 0;
    }

    resp->data = ptr;
    memcpy(&(resp->data[resp->size]), contents, realsize);
    resp->size += realsize;
    resp->data[resp->size] = 0;

    return realsize;
}

// Function to parse a single JSON response line
OllamaResponse* parse_response_line(const char* json_str) {
    OllamaResponse* resp = calloc(1, sizeof(OllamaResponse));
    if (!resp) {
        printf("Error: Memory allocation failed for response struct\n");
        return NULL;
    }

    cJSON *json = cJSON_Parse(json_str);
    if (!json) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr) {
            printf("Error parsing JSON: %s\n", error_ptr);
        }
        free(resp);
        return NULL;
    }

    // Parse model
    cJSON *model = cJSON_GetObjectItem(json, "model");
    if (cJSON_IsString(model) && model->valuestring != NULL) {
        resp->model = strdup(model->valuestring);
    }

    // Parse response text
    cJSON *response = cJSON_GetObjectItem(json, "response");
    if (cJSON_IsString(response) && response->valuestring != NULL) {
        resp->response = strdup(response->valuestring);
    }

    // Parse done status
    cJSON *done = cJSON_GetObjectItem(json, "done");
    if (cJSON_IsBool(done)) {
        resp->done = strdup(done->valueint ? "true" : "false");
    }

    // Parse timing information
    cJSON *total_duration = cJSON_GetObjectItem(json, "total_duration");
    if (cJSON_IsNumber(total_duration)) {
        resp->total_duration = total_duration->valuedouble;
    }

    cJSON *load_duration = cJSON_GetObjectItem(json, "load_duration");
    if (cJSON_IsNumber(load_duration)) {
        resp->load_duration = load_duration->valuedouble;
    }

    cJSON *prompt_eval_count = cJSON_GetObjectItem(json, "prompt_eval_count");
    if (cJSON_IsNumber(prompt_eval_count)) {
        resp->prompt_eval_count = prompt_eval_count->valuedouble;
    }

    cJSON *prompt_eval_duration = cJSON_GetObjectItem(json, "prompt_eval_duration");
    if (cJSON_IsNumber(prompt_eval_duration)) {
        resp->prompt_eval_duration = prompt_eval_duration->valuedouble;
    }

    cJSON *eval_count = cJSON_GetObjectItem(json, "eval_count");
    if (cJSON_IsNumber(eval_count)) {
        resp->eval_count = eval_count->valuedouble;
    }

    cJSON *eval_duration = cJSON_GetObjectItem(json, "eval_duration");
    if (cJSON_IsNumber(eval_duration)) {
        resp->eval_duration = eval_duration->valuedouble;
    }

    cJSON_Delete(json);
    return resp;
}

void free_ollama_response(OllamaResponse* resp) {
    if (resp) {
        free(resp->model);
        free(resp->response);
        free(resp->done);
        free(resp);
    }
}

// Function to extract response from JSON
char* extract_response(char* response_data) {
    int size = strlen(response_data);
    char* ptr;

    // Remove <think> and </think> tags
    char* result = strstr(response_data, "<think>");
    if (result) {
        result = strstr(result, "</think>");
        ptr = result + strlen("</think>");
        while (ptr < response_data + size && *ptr == '\n') {
            // skip '\n'
            ptr++;
        }
        if (ptr < response_data + size) {
            memmove(response_data, ptr, size - (ptr - response_data));
            size -= (ptr - response_data);
            response_data[size] = '\0';
        }
    }
    //printf("Extracting final response from JSON: %s\n", response_data);

    return response_data;
}

// Main function to send question to Ollama
char* ask_ollama(const char* question) {
    CURL *curl;
    CURLcode res;
    struct ResponseData response_data = {0};
    char *final_response = NULL;

    // Initialize response data
    response_data.data = malloc(1);
    response_data.size = 0;

    // Initialize CURL
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (!curl) {
        printf("Error: CURL initialization failed\n");
        free(response_data.data);
        return NULL;
    }

    // Prepare JSON payload
    char *json_payload = malloc(strlen(question) + 100);
    sprintf(json_payload, "{\"model\": \"deepseek-r1:1.5b\", \"prompt\": \"%s\"}", question);

    // Set CURL options
    curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:11434/api/generate");
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload);
    
    // Set headers
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Set write callback
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response_data);

    // Perform the request
    res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        printf("Error: %s\n", curl_easy_strerror(res));
    } else {
        // Extract response from JSON
        char *response_result = malloc(1);
        int response_result_size = 0;
        if (response_result == NULL) {
            printf("Error: Memory allocation failed for response_result\n");
            goto end;
        }

        bool done = false;
        
        char *line = strtok(response_data.data, "\n");
        while (line != NULL) {
            if (strlen(line) > 0) {
                OllamaResponse* resp = parse_response_line(line);
                if (resp) {
                    // copy each line of response content
                    if (resp->response) {
                        int current_size = response_result_size;
                        response_result_size += strlen(resp->response);
                        char* ptr = realloc(response_result, response_result_size+1);
                        if (ptr == NULL) {
                            printf("Error: Memory allocation failed for response_result\n");
                            free(response_result);
                            goto end;
                        }
                        response_result = ptr;
                        strcpy(&response_result[current_size], resp->response);
                    }
                    
                    // Print statistics when done
                    if (resp->done && strcmp(resp->done, "true") == 0) {
                        final_response = extract_response(response_result);
                        done = true;
                    }
                    
                    free_ollama_response(resp);

                    if (done) {
                        break;
                    }
                }
            }
            line = strtok(NULL, "\n");
        }

    }

end:
    // Cleanup
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    free(json_payload);
    free(response_data.data);

    return final_response;
}

