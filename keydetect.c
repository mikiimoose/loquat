#include "keydetect.h"
#include "audio_capture.h"


static pthread_t detect_thread;
static char device_path[MAX_DEVICE_PATH] = {0};
static int running = 1;

// Find the MSR keyboard device
// Returns -1 if no MSR keyboard device is found    
int find_msr_keyboard() {
    FILE *fp;
    char line[512];
    char *handlers;
    int event_num = -1;
    int smallest_event_num = -1; 
    
    // Open /proc/bus/input/devices
    fp = fopen("/proc/bus/input/devices", "r");
    if (fp == NULL) {
        log_message(LOG_ERR, "Failed to open /proc/bus/input/devices");
        return -1;
    }
    
    // Read file line by line
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strstr(line, "Vendor=" VENDOR_ID " Product=" PRODUCT_ID)) {
            // Found our device, now look for its event handler
            while (fgets(line, sizeof(line), fp) != NULL) {
                if (strstr(line, "Handlers=")) {
                    handlers = strstr(line, "event");
                    if (handlers) {
                        if (sscanf(handlers, "event%d", &event_num) == 1) {
                            // Update smallest_event_num if this is the first event or smaller than current
                            if (smallest_event_num == -1 || event_num < smallest_event_num) {
                                smallest_event_num = event_num;
                            }
                        }
                    }
                }
                if (line[0] == '\n') break; // End of device block
            }
            // Don't break here, continue searching for other matching devices
        }
    }
    fclose(fp);
    
    if (smallest_event_num >= 0) {
        snprintf(device_path, MAX_DEVICE_PATH, "/dev/input/event%d", smallest_event_num);
        return 0;
    }
    
    return -1;
}

// Thread function for F2 key detection
void* detect_keys(void *arg) {
    (void)arg;  // Unused parameter
    struct input_event ev;
    int fd;
    
    // Open the input device
    fd = open(device_path, O_RDONLY);
    if (fd == -1) { 
        log_message(LOG_ERR, "Failed to open input device: %s", strerror(errno));
        pthread_exit(NULL);
    }
    
    log_message(LOG_INFO, "Listening for F2 key events");
    
    while (running) {
        // Read input events with timeout
        fd_set rfds;
        struct timeval tv;
        int retval;

        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        
        // Set timeout to 1 second
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        retval = select(fd + 1, &rfds, NULL, NULL, &tv);
        
        if (retval == -1) {
            if (errno == EINTR) continue;  // Interrupted by signal
            log_message(LOG_ERR, "Select failed: %s", strerror(errno));
            break;
        }
        
        if (retval) {
            ssize_t bytes_read = read(fd, &ev, sizeof(struct input_event));
            if (bytes_read < 0) {
                if (errno == EINTR) continue;  // Interrupted by signal
                log_message(LOG_ERR, "Failed to read input event: %s", strerror(errno));
                break;
            }
            
            // Check for key events
            if (ev.type == EV_KEY) {
                if (ev.code == KEY_F2) {
                    if (ev.value == 1) { // Key press
                        log_message(LOG_INFO, "F2 key pressed");
                        start_audio_capture();

                    } else if (ev.value == 0) { // Key release
                        log_message(LOG_INFO, "F2 key released");
                        stop_audio_capture();
                    }
                }
            }
        }
    }
    
    close(fd);
    log_message(LOG_INFO, "Key detection thread terminated");
    pthread_exit(NULL);
}

// Initialize key detection
// Returns 0 on success, -1 on failure
int key_detection_initialize() {
        
    // Find the MSR keyboard device
    if (find_msr_keyboard() < 0) {
        log_message(LOG_ERR, "Could not find MSR keyboard device");
        return -1;
    }
    
    // Create detection thread
    if (pthread_create(&detect_thread, NULL, detect_keys, NULL) != 0) {
        log_message(LOG_ERR, "Failed to create detection thread: %s", strerror(errno));
        return -1;
    }

    return 0;
}

// Deinitialize key detection
void key_detection_deinitialize(void) {
    running = 0;
    pthread_join(detect_thread, NULL); 
}