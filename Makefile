CFLAGS += -std=c99

.PHONY: all clean

all: daemon-snake

daemon_snake: daemon-snake.c daemon.c

clean:
	rm -f daemon-snake
