#ifndef KEYDETECT_H
#define KEYDETECT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/input-event-codes.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/stat.h>
#include "logger.h"
#include "config.h"
#include "keydetect.h"
/*
// If you have a struct, declare it here:
typedef struct {
    // Add members as needed
    int fd;
    // ... other members ...
} KeyDetect;

// Function declarations
int keydetect_init(KeyDetect *kd);
void keydetect_deinit(KeyDetect *kd);
int keydetect_is_key_pressed(KeyDetect *kd, int key_code);
*/
// Or, if your API is stateless, just declare the functions:
int key_detection_initialize(void);
void key_detection_deinitialize(void);
//int keydetect_is_key_pressed(int key_code);

#endif // KEYDETECT_H 