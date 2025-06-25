#include <opencv2/opencv.hpp>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <pthread.h>
#include <atomic>
#include <unistd.h>
#include "cameradetect.h"
#include "snapshot.h"
#include "ai_common.h"
#include "tts_espeak.h"
#include "config.h"
#include "logger.h"

static pthread_t snapshot_thread;
static std::atomic<int> snapshot_requested(0);

static void* snapshot_thread_func(void* arg) {
    (void)arg;
    while (1) {
        if (snapshot_requested.exchange(0)) {
            // Take the snapshot
            struct stat st = {0};
            if (stat(DEFAULT_FOLDER, &st) == -1) {
                if (mkdir(DEFAULT_FOLDER, 0755) != 0) {
                    log_message(LOG_ERR, "Failed to create snapshot directory: %s", DEFAULT_FOLDER);
                    continue;
                }
            }

            char filename[256];
            snprintf(filename, sizeof(filename), "%s/snapshot.jpg", DEFAULT_FOLDER);

            if (!camera_capture(filename)) {
                log_message(LOG_ERR, "Failed to capture image.");
            } else {
                log_message(LOG_INFO, "Snapshot saved successfully at: %s", filename);
                
                log_message(LOG_INFO, "Asking ChatGPT about the image...");
                char* response = ask_chatgpt_with_image(NULL, filename);
                
                if (response) {
                    log_message(LOG_INFO, "AI Response: %s", response);
                    espeak(response);
                    free(response);
                } else {
                    log_message(LOG_ERR, "Failed to get a response from the AI.");
                    espeak("Failed to get a response from the AI.");
                }
            }
        }
        // Sleep briefly to avoid busy waiting
        usleep(10000); // 10ms
    }
    return NULL;
}

extern "C" {

// Call this once at startup
int snapshot_thread_initialize(void) {
    snapshot_requested = 0;
    if (pthread_create(&snapshot_thread, NULL, snapshot_thread_func, NULL) != 0) {
        log_message(LOG_ERR, "Failed to create snapshot thread");
        return -1;
    }
    return 0;
}

// Call this at shutdown
void snapshot_thread_deinitialize(void) {
    // Not strictly necessary unless you want to join/stop the thread
    // For now, just detach
    pthread_detach(snapshot_thread);
}

// Call this from keydetect.cpp when F3 is pressed
void trigger_snapshot(void) {
    snapshot_requested = 1;
}

}
