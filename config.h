#ifndef CONFIG_H_
#define CONFIG_H_

// Audio capture
#define SAMPLE_RATE 48000
#define FRAMES_PER_BUFFER 256
#define NUM_CHANNELS 1
#define OUTPUT_FILENAME "audio.wav"
#define AUDIO_CAPTURE_DEVICE "USB PnP Sound Device"

// MSR keyboard device
#define MAX_DEVICE_PATH 512
#define VENDOR_ID "5131"
#define PRODUCT_ID "2019"

#endif /* CONFIG_H_ */

