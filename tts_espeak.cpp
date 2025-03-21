#include <espeak-ng/speak_lib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include "tts_espeak.h"
 
static espeak_AUDIO_OUTPUT output = AUDIO_OUTPUT_SYNCH_PLAYBACK;
static char *path = NULL;
static void* user_data;
static unsigned int *identifier;
static int buflength = 5000;
static unsigned int position = 0, end_position = 0, flags = espeakCHARS_AUTO;
static int options = 0;
static espeak_POSITION_TYPE position_type = POS_CHARACTER;
    
void tts_init() {

	espeak_Initialize(output, buflength, path, options );
	espeak_VOICE voice;
	memset(&voice, 0, sizeof(espeak_VOICE)); // Zero out the voice first
	const char *langNativeString = "en"; // Set voice by properties
	voice.languages = langNativeString;
	voice.name = "US";
	voice.variant = 2;
	voice.gender = 2;
	espeak_SetVoiceByProperties(&voice);
}


int espeak(char* text) {
	//printf("Saying  '%s'...\n", text);
	espeak_Synth(text, buflength, position, position_type, end_position, flags, identifier, user_data);
    
    return 0;
}