CFLAGS += -std=c99

.PHONY: all clean

all: snaked tetrisd

snaked: snaked.c daemon.c

tetrisd: tetrisd.c daemon.c

clean:
	rm -f snaked tetrisd
