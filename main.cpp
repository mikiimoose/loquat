#include "keydetect.h"
#include <getopt.h>
#include <sys/prctl.h>
#include "audio_capture.h"
#include "tts_espeak.h"
#include "config.h"
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

// Global variables
static volatile sig_atomic_t running = 1;

// Signal handler to handle graceful shutdown
void signal_handler(int signo) {
    if (signo == SIGTERM || signo == SIGINT) {
        running = 0;
    }
}

void print_usage(const char *program_name) {
    fprintf(stderr, "Usage: %s [options]\n", program_name);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -c, --console    Output logs to console instead of syslog\n");
    fprintf(stderr, "  -h, --help       Display this help message\n");
}

// Deletes all files in the specified directory
// Returns 0 on success, -1 on error
int delete_files_in_directory(const char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        log_message(LOG_ERR, "Cannot open directory %s: %s", dir_path, strerror(errno));
        return -1;
    }

    struct dirent *entry;
    char file_path[PATH_MAX];
    
    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".." directories
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Construct full file path
        snprintf(file_path, PATH_MAX, "%s/%s", dir_path, entry->d_name);

        // Delete the file
        if (unlink(file_path) != 0) {
            log_message(LOG_ERR, "Failed to delete %s: %s", file_path, strerror(errno));
            closedir(dir);
            return -1;
        }
    }

    closedir(dir);
    return 0;
}

int main(int argc, char *argv[]) {
    log_output_t output_type = LOG_OUTPUT_SYSLOG;
    int opt;

    static struct option long_options[] = {
        {"console", no_argument,       0, 'c'},
        {"help",    no_argument,       0, 'h'},
        {0,         0,                 0,  0 }
    };
    
    while ((opt = getopt_long(argc, argv, "cdh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'c':
                output_type = LOG_OUTPUT_CONSOLE;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    // Initialize logging
    log_init("loquat", output_type);
    tts_init();
    
    // Set up signal handlers
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

    // Delete all old audio files
    char path[128];
    snprintf(path, 128, "%s/%s", DEFAULT_FOLDER, AUDIO_FOLDER);
    delete_files_in_directory(path);

    // 
    if (audio_capture_initialize() < 0) {
        log_message(LOG_ERR, "Failed to initialize audio capture");
        goto exit1;
    }

    // Initialize key detection
    if (key_detection_initialize() < 0) {
        log_message(LOG_ERR, "Failed to initialize key detection");
        goto exit2;
    }
    
    // Main loop
    while (running) {
        sleep(1);  // Sleep to prevent busy waiting
    }
    
    key_detection_deinitialize();
exit2:
    audio_capture_deinitialize();
exit1:
    log_cleanup();
    return 0;
} 
