#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "whisper.h"
#include "common.h"
#include "common-whisper.h"
#include "stt_whisper.h"
#include "config.h"
#include "logger.h"

//#ifdef __cplusplus
//extern "C" {
//#endif

// return text from WAV file
char* stt_whisper(char* wav_file) {
    char* text_result = NULL;
    int total_size = 0;
    char model[128];

    log_message(LOG_INFO, "enter whisper");
    snprintf(model, sizeof(model), "%s/%s/%s", DEFAULT_FOLDER, MODEL_FOLDER, WHISPER_MODEL_NAME);

    if (wav_file == NULL) {
        return NULL;
    }

    if (is_file_exist(wav_file) == false)
        return NULL;

    // Initialize whisper context with custom parameters
    struct whisper_context_params cparams = whisper_context_default_params();
    cparams.use_gpu = false;  // Enable GPU if available
    
    struct whisper_context* ctx = whisper_init_from_file_with_params(model, cparams);
    if (ctx == NULL) {
        log_message(LOG_ERR, "Failed to initialize whisper context, %s\n", model);
        return NULL;
    }

    // Initialize parameters with more options
    struct whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.print_realtime   = true;
    params.print_progress   = false;  // We'll use our own callback
    params.print_timestamps = true;
    params.language         = "en";
    params.translate        = false;
    params.n_threads       = 4;
    params.offset_ms       = 0;
    params.no_context      = true;
    params.single_segment  = false;
    params.max_tokens      = 0;
    params.audio_ctx       = 0;

    std::vector<float> pcmf32;               // mono-channel F32 PCM
    std::vector<std::vector<float>> pcmf32s; // stereo-channel F32 PCM

    if (!::read_audio_data(wav_file, pcmf32, pcmf32s, false)) {
        log_message(LOG_ERR, "error: failed to read WAV file '%s'\n", wav_file);
        whisper_free(ctx);
        return NULL;
    }

    // Load and process audio
    if (whisper_full_parallel(ctx, params, pcmf32.data(), pcmf32.size(), 1) != 0) {
        log_message(LOG_ERR, "Failed to process audio\n");
        whisper_free(ctx);
        return NULL;
    }

    // Get and print results
    const int n_segments = whisper_full_n_segments(ctx);
    
    for (int i = 0; i < n_segments; i++) {
        const char* text = whisper_full_get_segment_text(ctx, i);
        int size = strlen(text);

        // Reallocate buffer to fit new segment
        char* new_result = (char*)realloc(text_result, total_size + size + 1);
        if (new_result == NULL) {
            free(text_result);
            whisper_free(ctx);
            return NULL;
        }
        text_result = new_result;
        memcpy(text_result+total_size, text, size);
        total_size += size;
        text_result[total_size] = '\0';
    }

    whisper_free(ctx);
    return text_result;  // Caller is responsible for freeing this memory
}

//#ifdef __cplusplus
//}
//#endif

