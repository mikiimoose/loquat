#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <cjson/cJSON.h>
#include "ai_common.h"

/*
gets chatgpt api key from chatgpt.conf
return -1 means error
return 0 means success
*/
int getApikey(char *buff, int size) {
    int fd;
    ssize_t nread;

    char* path = "chatgpt.conf";
    fd = open(path, O_RDONLY);
    if (fd == -1) {
        printf("Error: Could not open file\n");
        return -1;
    }
    
    nread = read(fd, buff, size);
    //printf("Read %ld\n", nread);
    //printf("Content: %s\n", buff);

    close(fd);
    if (nread <= 0) {
        return -1;
    }
    return 0;
}

/* extracts the response from the json object
return NULL if error. Otherwise, return the response buffer. User must free the buffer.
*/
char* extractResponse(char* ptr) {
    cJSON *root = cJSON_Parse(ptr); // parse the json response
    if (root == NULL) {
        printf("Error: Could not parse json\n");
        return NULL;
    }

    cJSON *choices = cJSON_GetObjectItem(root, "choices"); // get the choices object
    if (choices == NULL) {
        printf("Error: Could not get choices\n");
        cJSON_Delete(root);
        return NULL;
    }

    cJSON *choice = cJSON_GetArrayItem(choices, 0); // get the first choice
    if (choice == NULL) {
        printf("Error: Could not get choice\n");
        cJSON_Delete(root);
        return NULL;
    }

    cJSON *message = cJSON_GetObjectItem(choice, "message"); // get the text object
    if (message == NULL) {
        printf("Error: Could not get message\n");
        cJSON_Delete(root);
        return NULL;
    }

    cJSON *content = cJSON_GetObjectItem(message, "content"); // get the text object
    if (content == NULL) {
        printf("Error: Could not get content\n");
        cJSON_Delete(root);
        return NULL;
    }

    char* result = strdup(content->valuestring); // get the value of the text object
    cJSON_Delete(root);
    return result;
}

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

/*
sends a question to chatgpt and prints the response
return NULL if error. Otherwise, return the response buffer. User must free the buffer.
*/
char* ask_chatgpt(char* question) {
    CURL *curl;
    CURLcode res = CURLE_OK;
    char* url = "https://api.openai.com/v1/chat/completions";
    char api_key[512];
    struct ResponseData response_data = {0};
    char *final_response = NULL;

    memset(api_key, 0, sizeof(api_key));

    int ret = getApikey(api_key, sizeof(api_key));
    if (ret == -1) {
        printf("Error: Could not get api key\n");
        return NULL;
    }
    //printf("Api key: %s\n", api_key);
    //printf("Api key length: %ld\n", strlen(api_key));

    char data[2500];
    snprintf(data, sizeof(data), "{\"model\": \"gpt-3.5-turbo\", \"messages\": [{\"role\": \"user\", \"content\": \"%s\"}]}", question);
    //printf("Data: %s\n", data);

    // Initialize response data
    response_data.data = malloc(1);
    response_data.size = 0;
    
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (!curl)
    {
        printf("Error: Could not initialize cURL.\n");
        return NULL;
    }

    struct curl_slist *headers = NULL;
    char auth_header[1024];

    headers = curl_slist_append(headers, "Content-Type: application/json");
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", api_key);
    headers = curl_slist_append(headers, auth_header);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(data));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response_data);
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);
    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    if ((res = curl_easy_perform(curl)) != CURLE_OK) {
        printf("Error: %s\n", curl_easy_strerror(res));
    } else {
        final_response = extractResponse(response_data.data);
    }

    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    free(response_data.data);
    return final_response;
}