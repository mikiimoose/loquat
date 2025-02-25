#ifndef AUDIO_CAPTURE_H
#define AUDIO_CAPTURE_H
#include <portaudio.h>
#include <sndfile.h>
#include "config.h"

typedef struct {
    SNDFILE *sndfile;
    SF_INFO sfinfo;
    short *buffer;
} AudioData;

typedef struct {
    char riff_header[4];    // "RIFF"
    int wav_size;           // Size of WAV
    char wave_header[4];    // "WAVE"
    char fmt_header[4];     // "fmt "
    int fmt_chunk_size;     // Size of fmt chunk
    short audio_format;     // Audio format
    short num_channels;     // Number of channels
    int sample_rate;        // Sample rate
    int byte_rate;         // Byte rate
    short sample_alignment; // Sample alignment
    short bit_depth;       // Bit depth
    char data_header[4];    // "data"
    int data_bytes;        // Size of data section
} WavHeader;

void start_audio_capture();
void stop_audio_capture();

#endif // AUDIO_CAPTURE_H 