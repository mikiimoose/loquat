#ifndef SPEAKER_DETECT_H
#define SPEAKER_DETECT_H

#include <stdio.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>
#include "keydetect.h"

#ifdef __cplusplus
extern "C" {
#endif

// Constants for speaker identification
#define SPEAKER_VENDOR_ID  0x4c4a   // Replace with your speaker's vendor ID
#define SPEAKER_PRODUCT_ID 0x4155   // Replace with your speaker's product ID

/**
 * Initialize and detect the USB speaker
 * 
 * @return 1 if speaker is found and initialized successfully, 0 otherwise
 */
int speaker_init(void);

#ifdef __cplusplus
}
#endif

#endif /* SPEAKER_DETECT_H */
