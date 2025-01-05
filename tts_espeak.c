#include <espeak-ng/speak_lib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
 
static espeak_AUDIO_OUTPUT output = AUDIO_OUTPUT_SYNCH_PLAYBACK;
static char *path = NULL;
static void* user_data;
static unsigned int *identifier;
 
int espeak(char* text) {
    char voicename[] = {"English"}; // Set voice by its name
    
    int buflength = 500, options = 0;
    unsigned int position = 0, position_type = 0, end_position = 0, flags = espeakCHARS_AUTO;
    
    
    espeak_Initialize(output, buflength, path, options);
    espeak_SetVoiceByName(voicename);
    
    //printf("Saying  '%s'...\n", text);
    espeak_Synth(text, buflength, position, position_type, end_position, flags, identifier, user_data);
    
    return 0;
}