#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <cjson/cJSON.h>
#include "ai_common.h"
#include "config.h"
#include <vector>
#include <string>
#include <fstream>
#include <streambuf>

/*
gets chatgpt api key from chatgpt.conf
return -1 means error
return 0 means success
*/
int getApikey(char *buff, int size) {
    int fd;
    ssize_t nread;
    char path[128];

    snprintf(path, sizeof(path), "%s/%s", DEFAULT_FOLDER, CHATGPT_API_KEY_FILE);
    printf("DEBUG: Attempting to open API key file at: %s\n", path);
    fd = open(path, O_RDONLY);
    if (fd == -1) {
        printf("Error: Could not open file: %s\n", strerror(errno));
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
    printf("DEBUG: Raw API response: %s\n", ptr);
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

    char *ptr = (char*)realloc(resp->data, resp->size + realsize + 1);
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
    response_data.data = (char*)malloc(1);
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

// Helper function to encode binary data (like an image) to base64
static std::string base64_encode(const std::vector<unsigned char>& data) {
    static const char* b64_table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string encoded_string;
    int val = 0, valb = -6;
    for (unsigned char c : data) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            encoded_string.push_back(b64_table[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) {
        encoded_string.push_back(b64_table[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    while (encoded_string.size() % 4) {
        encoded_string.push_back('=');
    }
    return encoded_string;
}

/*
sends a question and an image to chatgpt and prints the response
return NULL if error. Otherwise, return the response buffer. User must free the buffer.
*/
char* ask_chatgpt_with_image(const char* question, const char* image_path) {
    CURL *curl;
    CURLcode res = CURLE_OK;
    char api_key[512];
    struct ResponseData response_data = {0};
    char *final_response = NULL;

    // If no question is provided, use the default medical question.
    const char* final_question = question;
    if (question == NULL || strlen(question) == 0) {
        final_question = "What are the medical implications found in this photo?";
    }

    // 1. Get API Key
    memset(api_key, 0, sizeof(api_key));
    if (getApikey(api_key, sizeof(api_key)) == -1) {
        printf("Error: Could not get api key\n");
        return NULL;
    }

    // 2. Read and Base64 Encode the Image
    std::ifstream file(image_path, std::ios::binary);
    if (!file) {
        fprintf(stderr, "Error: Failed to open image file: %s\n", image_path);
        return NULL;
    }
    std::vector<unsigned char> image_buffer(std::istreambuf_iterator<char>(file), {});
    std::string base64_image = base64_encode(image_buffer);
    
    // 3. Construct JSON payload for Vision Model
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "model", "gpt-4o");
    cJSON_AddNumberToObject(root, "max_tokens", 300);

    cJSON *messages_array = cJSON_CreateArray();
    cJSON *message_object = cJSON_CreateObject();
    cJSON_AddStringToObject(message_object, "role", "user");

    cJSON *content_array = cJSON_CreateArray();
    // Text part
    cJSON *text_part = cJSON_CreateObject();
    cJSON_AddStringToObject(text_part, "type", "text");
    cJSON_AddStringToObject(text_part, "text", final_question); // Using the determined question
    cJSON_AddItemToArray(content_array, text_part);
    // Image part
    cJSON *image_part = cJSON_CreateObject();
    cJSON_AddStringToObject(image_part, "type", "image_url");
    cJSON *image_url_object = cJSON_CreateObject();
    std::string data_url = "data:image/jpeg;base64," + base64_image;
    cJSON_AddStringToObject(image_url_object, "url", data_url.c_str());
    cJSON_AddItemToObject(image_part, "image_url", image_url_object);
    cJSON_AddItemToArray(content_array, image_part);

    cJSON_AddItemToObject(message_object, "content", content_array);
    cJSON_AddItemToArray(messages_array, message_object);
    cJSON_AddItemToObject(root, "messages", messages_array);

    char* json_payload = cJSON_PrintUnformatted(root);
    
    // 4. Perform cURL Request
    response_data.data = (char*)malloc(1);
    response_data.size = 0;
    
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (!curl) {
        printf("Error: Could not initialize cURL.\n");
        cJSON_Delete(root);
        free(json_payload);
        free(response_data.data);
        return NULL;
    }

    struct curl_slist *headers = NULL;
    char auth_header[1024];
    headers = curl_slist_append(headers, "Content-Type: application/json");
    snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", api_key);
    headers = curl_slist_append(headers, auth_header);

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/chat/completions");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response_data);

    if ((res = curl_easy_perform(curl)) != CURLE_OK) {
        printf("Error: %s\n", curl_easy_strerror(res));
    } else {
        final_response = extractResponse(response_data.data);
    }

    // 5. Cleanup
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    cJSON_Delete(root);
    free(json_payload);
    free(response_data.data);

    printf("DEBUG: Final response: %s\n", final_response);
    return final_response;
}
