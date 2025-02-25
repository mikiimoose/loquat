#include "keydetect.h"
#include <getopt.h>
#include <sys/capability.h>
#include <sys/prctl.h>

// Global variables
static volatile sig_atomic_t running = 1;

static int setup_capabilities(void) {
    cap_t caps;
    cap_value_t cap_list[] = {CAP_DAC_OVERRIDE};
    
    // Initialize capabilities
    caps = cap_get_proc();
    if (caps == NULL) {
        log_message(LOG_ERR, "Failed to get capabilities: %s", strerror(errno));
        return -1;
    }
    
    // Clear all capabilities
    if (cap_clear(caps) == -1) {
        log_message(LOG_ERR, "Failed to clear capabilities: %s", strerror(errno));
        cap_free(caps);
        return -1;
    }
    
    // Set specific capabilities we need
    if (cap_set_flag(caps, CAP_PERMITTED, 1, cap_list, CAP_SET) == -1) {
        log_message(LOG_ERR, "Failed to set permitted capabilities: %s", strerror(errno));
        cap_free(caps);
        return -1;
    }
    
    if (cap_set_flag(caps, CAP_EFFECTIVE, 1, cap_list, CAP_SET) == -1) {
        log_message(LOG_ERR, "Failed to set effective capabilities: %s", strerror(errno));
        cap_free(caps);
        return -1;
    }
    
    // Apply the capabilities
    if (cap_set_proc(caps) == -1) {
        log_message(LOG_ERR, "Failed to apply capabilities: %s", strerror(errno));
        cap_free(caps);
        return -1;
    }
    
    // Free the capability state
    if (cap_free(caps) == -1) {
        log_message(LOG_ERR, "Failed to free capabilities: %s", strerror(errno));
        return -1;
    }
    
    return 0;
}

void daemonize(void) {
    pid_t pid;

    // First fork (detaches from parent)
    pid = fork();
    if (pid < 0) {
        log_message(LOG_ERR, "Failed to fork first time");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS); // Parent exits
    }

    // Child process continues
    if (setsid() < 0) { // Create new session
        log_message(LOG_ERR, "Failed to create new session");
        exit(EXIT_FAILURE);
    }

    // Second fork (relinquish session leadership)
    pid = fork();
    if (pid < 0) {
        log_message(LOG_ERR, "Failed to fork second time");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    // Change working directory
    if (chdir("/") < 0) {
        log_message(LOG_ERR, "Failed to change working directory");
        exit(EXIT_FAILURE);
    }

    // Reset file creation mask
    umask(0);

    // Close all open file descriptors
    for (int x = sysconf(_SC_OPEN_MAX); x >= 0; x--) {
        close(x);
    }
}

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
    fprintf(stderr, "  -d, --daemon     Run as daemon\n");
    fprintf(stderr, "  -h, --help       Display this help message\n");
}

// Add it later
#if 0 

int main(int argc, char *argv[]) {
    log_output_t output_type = LOG_OUTPUT_SYSLOG;
    int opt;
    int daemon_mode = 0;
    
    static struct option long_options[] = {
        {"console", no_argument,       0, 'c'},
        {"daemon",  no_argument,       0, 'd'},
        {"help",    no_argument,       0, 'h'},
        {0,         0,                 0,  0 }
    };
    
    while ((opt = getopt_long(argc, argv, "cdh", long_options, NULL)) != -1) {
        switch (opt) {
            case 'c':
                output_type = LOG_OUTPUT_CONSOLE;
                break;
            case 'd':
                daemon_mode = 1;
                output_type = LOG_OUTPUT_SYSLOG; // Force syslog in daemon mode
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
    log_init("s2t", output_type);
    
    // Set up capabilities before anything else
    if (setup_capabilities() < 0) {
        log_message(LOG_ERR, "Failed to set up capabilities");
        log_cleanup();
        return 1;
    }
    
    if (daemon_mode) {
        log_message(LOG_INFO, "Starting daemon mode");
        daemonize();
        log_message(LOG_INFO, "Daemon started");
    }
    
    // Set up signal handlers
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);

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
#endif 
