CFLAGS += -std=c99

.PHONY: all clean

all: snake-daemon tetris-daemon

snake-daemon: snake-daemon.c daemon.c

tetris-daemon: tetris-daemon.c daemon.c

clean:
	rm -f snake-daemon tetris-daemon
