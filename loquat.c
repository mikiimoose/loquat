#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <cjson/cJSON.h>

#define BUFF_SIZE 2048
// global variable to store the response
static char* answer = NULL;

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

size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t realsize = size * nmemb;
    if (realsize == 0 || ptr == NULL) {
        answer = NULL;
        return 0;
    }

    answer = malloc(realsize + 1);
    if (answer == NULL) {
        printf("Error: Could not allocate memory\n");
        return 0;
    }
    memset(answer, 0, realsize + 1);
    memcpy(answer, ptr, realsize);

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
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_2);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);
    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    if ((res = curl_easy_perform(curl)) != CURLE_OK) {
        printf("Error: %s\n", curl_easy_strerror(res));
    }
    curl_slist_free_all(headers);

    curl_easy_cleanup(curl);
    curl_global_cleanup();

    return answer;
}



int main(int argc, char* argv[]) {
    char question[BUFF_SIZE] = {0};
    int size = 0;

    memset(question, 0, sizeof(question));

    for (int i=1; i<argc; i++) {
	    size += snprintf(question + size, BUFF_SIZE - size, "%s", argv[i]);
        if (i < argc - 1) {
            size += snprintf(question + size, BUFF_SIZE - size, " ");
        }
    }

    //printf("Prompt: %s\n", question);
    if (answer != NULL) {
        free(answer);
        answer = NULL;
    }

    answer = ask_chatgpt(question);
    if (answer == NULL) {
        printf("Error: Could not get response\n");
        return -1;
    }

    char* response = extractResponse(answer);
    if (response == NULL) {
        printf("Error: Could not extract response\n");
        free(answer);
        return -1;
    }

    printf("%s\n", response);
    espeak(response);
    
    free(response);
    free(answer);

    return 0;


}