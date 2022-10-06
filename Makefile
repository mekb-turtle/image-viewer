CC=cc
CFLAGS=-Wall -O2
LFLAGS=-lm -s -lSDL2

OBJS=image-viewer.o
TARGET=image-viewer

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LFLAGS)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -fv -- $(OBJS) $(TARGET)
