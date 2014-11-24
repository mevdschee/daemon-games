CFLAGS += -std=c99

.PHONY: all clean

all: daemon_snake

daemon_snake: daemon_snake.c daemon.c

clean:
	rm -f daemon_snake
