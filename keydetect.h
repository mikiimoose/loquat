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

int key_detection_initialize(void);
void key_detection_deinitialize(void);
#endif // KEYDETECT_H 