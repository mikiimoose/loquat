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

// MISC
#define FILE_NAME_LENGTH 64

#define DEFAULT_FOLDER	"/var/lib/loquat"

// Folder for captured audio wav file
#define AUDIO_FOLDER "audios"

#define MODEL_FOLDER "models"
// Whisper model name
#define WHISPER_MODEL_NAME "ggml-base.en.bin"

#define CHATGPT_API_KEY_FILE	"chatgpt.conf"

#define DEFAULT_PULSE_SINK	"alsa_output.usb-Jieli_Technology_UACDemoV1.0_4150344B3237310C-00.analog-stereo.monitor"

#endif /* CONFIG_H_ */

