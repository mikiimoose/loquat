TARGET = loquat
LIBS = -lcurl -lcjson -lespeak-ng
CC = gcc
CFLAGS = -g -Wall

SOURCE = loquat.o tts_espeak.o

.PHONY: all

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(SOURCE) $(CFLAGS) $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)
