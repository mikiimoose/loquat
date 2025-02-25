#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "keydetect.h"
#include "audio_capture.h"

static int running = 1;
static pthread_t record_thread;

static char* get_audio_filename(void) {
    time_t now;
    struct tm *tm_info;
    char *filename = malloc(32);  // Allocate memory for the filename
    
    if (filename == NULL) {
        return NULL;  // Handle allocation failure
    }
    
    time(&now);
    tm_info = localtime(&now);
    
    strftime(filename, 32, "audio_%Y%m%d_%H%M%S.wav", tm_info);
    return filename;
}

int convert_wav_sample_rate(const char* input_file, const char* output_file) {
    FILE *in_file = fopen(input_file, "rb");
    if (!in_file) {
        printf("Could not open input file\n");
        return -1;
    }

    // Read WAV header
    WavHeader header;
    if (fread(&header, sizeof(WavHeader), 1, in_file) != 1) {
        printf("Could not read WAV header\n");
        fclose(in_file);
        return -1;
    }

    // Verify it's a valid WAV file
    if (strncmp(header.riff_header, "RIFF", 4) != 0 || 
        strncmp(header.wave_header, "WAVE", 4) != 0) {
        printf("Invalid WAV file format\n");
        fclose(in_file);
        return -1;
    }

    // Verify input sample rate is 48000
    if (header.sample_rate != 48000) {
        printf("Input file must have 48000 Hz sample rate\n");
        fclose(in_file);
        return -1;
    }

    if (strncmp(header.data_header, "data", 4) != 0) {
        printf("data header does not have data, having %s\n", header.data_header);
        // Read and discard until we find the data header
        char buffer[128] = {0};
        fread(buffer, header.data_bytes, 1, in_file);

        while (1) {
            int n = fread(buffer, 1, 4, in_file);
            if  (n != 4) {
                printf("Could not find data header\n");
                fclose(in_file);
                return -1;
            }

            if (strncmp(buffer, "data", 4) == 0) {
                break;
            }
        }
        memcpy(header.data_header, "data", 4);

        // Read data size
        if (fread(&header.data_bytes, sizeof(int), 1, in_file) != 1) {
            printf("Could not read data size\n");
            fclose(in_file);
            return -1;
        }
    }

    printf("Input WAV header:\n");
    printf("  RIFF header: %s\n", header.riff_header);
    printf("  WAV header: %s\n", header.wave_header);
    printf("  Sample rate: %d\n", header.sample_rate);
    printf("  Byte rate: %d\n", header.byte_rate);
    printf("  Sample alignment: %d\n", header.sample_alignment);
    printf("  Bit depth: %d\n", header.bit_depth);
    printf("  Number of channels: %d\n", header.num_channels);
    printf("  Audio format: %d\n", header.audio_format);
    printf("  WAV size: %d\n", header.wav_size);
    printf("  FMT chunk size: %d\n", header.fmt_chunk_size);
    printf("  Data header: %s\n", header.data_header);
    printf("  Data bytes: %d\n", header.data_bytes);

    // Create output header
    WavHeader out_header = header;
    out_header.sample_rate = 16000;
    out_header.byte_rate = header.byte_rate / 3;
    out_header.data_bytes = header.data_bytes / 3;
    out_header.wav_size = out_header.data_bytes + sizeof(WavHeader) - 8;

    // Open output file
    FILE *out_file = fopen(output_file, "wb");
    if (!out_file) {
        printf("Could not create output file\n");
        fclose(in_file);
        return -1;
    }

    // Write output header
    fwrite(&out_header, sizeof(WavHeader), 1, out_file);

    // Process audio data
    short sample;
    int sample_count = 0;
    while (fread(&sample, sizeof(short), 1, in_file) == 1) {
        if (sample_count % 3 == 0) { // Take every third sample
            fwrite(&sample, sizeof(short), 1, out_file);
        }
        sample_count++;
    }

    fclose(in_file);
    fclose(out_file);
    return 0;
}


static int recordCallback(const void *inputBuffer, void *outputBuffer,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo* timeInfo,
                         PaStreamCallbackFlags statusFlags,
                         void *userData) {
    AudioData *data = (AudioData*)userData;
    const short *in = (const short*)inputBuffer;
    (void) outputBuffer; // Prevent unused variable warning
    (void) timeInfo; // Prevent unused variable warning
    (void) statusFlags; // Prevent unused variable warning
    
    if (in != NULL) {
        sf_writef_short(data->sndfile, in, framesPerBuffer);
    }
    
    return paContinue;
}

void* record_audio(void *userData) {
    PaStream *stream;
    PaError err;
    PaStreamParameters inputParameters;
    AudioData data;
    (void) userData;

    char *filename = get_audio_filename();
    if (filename == NULL) {
        log_message(LOG_ERR, "Failed to allocate memory for filename");
        return NULL;
    }
    
    // Set up sound file
    memset(&data.sfinfo, 0, sizeof(data.sfinfo));
    data.sfinfo.samplerate = SAMPLE_RATE;
    data.sfinfo.channels = NUM_CHANNELS;
    data.sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    
    data.sndfile = sf_open(filename, SFM_WRITE, &data.sfinfo);
    if (!data.sndfile) {
        log_message(LOG_ERR, "Could not open sound file for writing");
        free(filename);
        return NULL;
    }
    
    int numDevices = Pa_GetDeviceCount();
    if (numDevices <= 0) {
        log_message(LOG_ERR, "No audio capture devices found");
        sf_close(data.sndfile);
        free(filename);
        return NULL;
    }

    inputParameters.device = paNoDevice;

#ifdef AUDIO_CAPTURE_DEVICE
    // If AUDIO_CAPTURE_DEVICE is defined, use it as the audio capture device
    const PaDeviceInfo* deviceInfo;
    for (int i = 0; i < numDevices; i++) {
        deviceInfo = Pa_GetDeviceInfo(i);
        log_message(LOG_INFO, "Device %d: %s", i, deviceInfo->name);
        if (strstr(deviceInfo->name, AUDIO_CAPTURE_DEVICE) != NULL) {
            inputParameters.device = i;
            break;
        }
    }

    if (inputParameters.device == paNoDevice) {
        log_message(LOG_ERR, "No specified audio capture device found");
        //sf_close(data.sndfile);
        //return NULL;
    }
#endif

    inputParameters.device = Pa_GetDefaultInputDevice(); /* default input device */
    if (inputParameters.device == paNoDevice) {
        log_message(LOG_ERR, "No default audio capture device found");
        sf_close(data.sndfile);
        free(filename);
        return NULL;  
    }
    inputParameters.channelCount = NUM_CHANNELS;
    inputParameters.sampleFormat = paInt16;
    inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = NULL;

    err = Pa_OpenStream(
                &stream,
                &inputParameters,
                NULL,                  /* &outputParameters, */
                SAMPLE_RATE,
                FRAMES_PER_BUFFER,
                paClipOff,      /* we won't output out of range samples so don't bother clipping them */
                recordCallback,
                &data );
    
    if (err != paNoError) {
        log_message(LOG_ERR, "Failed to open audio stream: %s", Pa_GetErrorText(err));
        sf_close(data.sndfile);
        free(filename);
        return NULL;
    }

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        log_message(LOG_ERR, "Failed to start audio stream: %s", Pa_GetErrorText(err));
        Pa_CloseStream(stream);
        sf_close(data.sndfile);
        free(filename);
        running = 0;
        return NULL;
    }

    while (running) {
        Pa_Sleep(500);
    }
    // Stop and clean up
    err = Pa_StopStream(stream);
    if (err != paNoError) {
        log_message(LOG_ERR, "Failed to stop audio stream: %s", Pa_GetErrorText(err));
    }
    
    Pa_CloseStream(stream);
    sf_close(data.sndfile);
    // ReSample the WAV file to 16000 Hz
    char *output_filename = malloc(32);
    if (output_filename == NULL) {
        log_message(LOG_ERR, "Failed to allocate memory for output filename");
        free(filename);
        pthread_exit(NULL);
        return NULL;
    }
    strcpy(output_filename, filename);
    // Remove the extension
    char *ext = strrchr(output_filename, '.');
    if (ext != NULL) {
        *ext = '\0';
    }
    strcat(output_filename, "_16k.wav");
    convert_wav_sample_rate(filename, output_filename);
    free(filename);
    free(output_filename);

    pthread_exit(NULL);

    return NULL;
} 


void start_audio_capture() {
    running = 1;
    pthread_create(&record_thread, NULL, record_audio, NULL);
   
}

void stop_audio_capture(void) {
    running = 0;
    pthread_join(record_thread, NULL);
    log_message(LOG_INFO, "Audio capture stopped");
}


// Initialize audio capture
// Returns 0 on success, -1 on failure
int audio_capture_initialize(void) {
    PaError err;

    // Initialize PortAudio
    err = Pa_Initialize();
    if (err != paNoError) {
        log_message(LOG_ERR, "PortAudio initialization failed: %s", Pa_GetErrorText(err));
        return -1;
    }

    // Check if audio capture is supported
    if (Pa_GetDeviceCount() == 0) {
        log_message(LOG_ERR, "No audio capture devices found");
        Pa_Terminate();
        return -1;
    }


    return 0;
}

void audio_capture_deinitialize(void) {
    Pa_Terminate();
    return;
}