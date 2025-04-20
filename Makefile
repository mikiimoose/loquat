CPLUSPLUS = g++
CC = gcc
TARGET = loquat

CFLAGS = -g -Wall -Wextra
LDFLAGS = -pthread -lcap -lcurl -lcjson -lespeak-ng -lusb-1.0
LIBS_DIR = -L./libs
INCLUDES = -I. -I./include 

CFLAGS += $(shell pkg-config --cflags portaudio-2.0 sndfile)
LDFLAGS += $(shell pkg-config --libs portaudio-2.0 sndfile)

SRCS = tts_espeak.cpp ai_chatgpt.cpp ai_local.cpp network.cpp
SRCS += keydetect.cpp audio_capture.cpp logger.cpp main.cpp handling.cpp speaker_detect.cpp
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
