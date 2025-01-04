TARGET = loquat
LIBS = -lcurl -lcjson
CC = gcc
CFLAGS = -g -Wall

SOURCE = loquat.o

.PHONY: all

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(SOURCE) $(CFLAGS) $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)
