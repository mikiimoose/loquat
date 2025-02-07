#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "ai_common.h"
#include "tts_espeak.h"
#include "network.h"


#define BUFF_SIZE 2048

char* ask_ai(char* question) {

    char* answer = NULL;
    if (check_online_status() == 1) {
        answer = ask_chatgpt(question);
    } else {
        answer = ask_ollama(question);
    }
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

    char* response = ask_ai(question);
    if (response == NULL) {
        printf("Error: Could not get response\n");
        return -1;
    }


    printf("%s\n", response);
    espeak(response);
    
    free(response);

    return 0;


}