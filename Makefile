TARGET = loquat
LIBS = -lcurl -lcjson -lespeak-ng
CC = gcc
CFLAGS = -g -Wall

CFLAGS += $(shell pkg-config --cflags portaudio-2.0 sndfile)
LIBS += $(shell pkg-config --libs portaudio-2.0 sndfile)

SOURCE = loquat.o tts_espeak.o ai_chatgpt.o ai_local.o network.o

.PHONY: all

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(SOURCE) $(CFLAGS) $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)
