/*
 ============================================================================
 Name        : tetrisd.c
 Description : Multi-user console tetris game for GNU/Linux
 Author      : Maurits van der Schee <maurits@vdschee.nl>
 URL         : https://github.com/mevdschee/daemon-games
 ============================================================================
 */
#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "daemon.h"

void on_tick(daemon_t *daemon, int tick)
{
	//printf("%d\n",tick);
}

void on_data(daemon_t *daemon, int client)
{
	int nbytes;
	char bytes[128];

	nbytes = daemon->read(daemon, client, bytes, sizeof(bytes));
	if (nbytes) {
		if (bytes[0]=='q') {
			daemon->disconnect(daemon, client);
		} else {
			daemon->write(daemon, client, bytes, nbytes);
		}
	}
}

void on_connect(daemon_t *daemon, int client)
{
	int nbytes;
	char *bytes;

	bytes = "\e[30m\e[41m''\e[42m: \e[42m  \e[0m\e[30m\n\e[41m  \e[41m  \e[42m  \e[0m";
	nbytes = strlen(bytes);

	daemon->write(daemon,client,bytes,nbytes);
}

void on_disconnect(daemon_t *daemon, int client)
{
	printf("disconnect %d\n",client);
}

int main(int argc, char ** argv)
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s [port]\n",argv[0]);
		return EXIT_FAILURE;
	}

	int port = atoi(argv[1]);

	if (!port) {
		fprintf(stderr, "Invalid port number\n");
		return EXIT_FAILURE;
	}

	daemon_t *daemon = daemon_create(0,port,2,10);

	daemon->on_connect = on_connect;
	daemon->on_disconnect = on_disconnect;
	daemon->on_data = on_data;
	daemon->on_tick = on_tick;

	return daemon_run(daemon)?EXIT_SUCCESS:EXIT_FAILURE;
}
