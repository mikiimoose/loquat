CPLUSPLUS = g++
CC = gcc
TARGET = loquat

# Add architecture detection
ARCH ?= rpi
LIBS_DIR = -L./libs

# Add x86 target
.PHONY: x86
x86:
	$(MAKE) ARCH=x86 $(TARGET)

# Override libcommon based on architecture
ifeq ($(ARCH),x86)
    LIBS_DIR += -L./libs/x86
else
    LIBS_DIR += -L./libs/rpi
endif

LDFLAGS = -pthread -lcap -lwhisper -lcommon -lggml -lggml-base -lggml-cpu -lcurl -lcjson -lespeak-ng -lusb-1.0 -lopencv_core -lopencv_imgcodecs -lopencv_videoio
CFLAGS = -g -Wall -Wextra
CFLAGS += $(shell pkg-config --cflags portaudio-2.0 sndfile opencv4)
LDFLAGS += $(shell pkg-config --libs portaudio-2.0 sndfile opencv4)
INCLUDES = -I. -I./include -I./include/ggml/include

SRCS = tts_espeak.cpp ai_chatgpt.cpp ai_local.cpp network.cpp
SRCS += stt_whisper.cpp keydetect.cpp audio_capture.cpp logger.cpp main.cpp handling.cpp speaker_detect.cpp snapshot.cpp cameradetect.cpp
OBJS = $(SRCS:.cpp=.o)
OBJS := $(OBJS:.c=.o)

# Main target
$(TARGET): $(OBJS)
	$(CPLUSPLUS) $(OBJS) -o $(TARGET) $(LIBS_DIR) $(LDFLAGS)

# Compile C++ source files
%.o: %.cpp
	$(CPLUSPLUS) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile C source files
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJS) $(TARGET)

install: loquat
	@echo "Installing Loquat..."
	@sudo install -m 755 loquat /usr/local/bin/
	@sudo install -m 644 loquat.service /lib/systemd/system/
	@sudo mkdir -p /var/lib/loquat
	@sudo chown ${USER}:${USER} /var/lib/loquat
	@sudo chmod 755 /var/lib/loquat
	@sudo mkdir -p /var/lib/loquat/audios
	@sudo chown ${USER}:${USER} /var/lib/loquat/audios
	@sudo chmod 755 /var/lib/loquat/audios
	@sudo mkdir -p /var/lib/loquat/models
	@sudo chown ${USER}:${USER} /var/lib/loquat/models
	@sudo chmod 755 /var/lib/loquat/models
	@sudo systemctl daemon-reload
	@echo "Installation complete. You can now start the service with:"
	@echo "sudo systemctl enable loquat"
	@sudo systemctl enable loquat
	@echo "sudo systemctl start loquat"

.PHONY: clean
