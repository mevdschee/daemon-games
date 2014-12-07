/*
 ============================================================================
 Name        : lobby.c
 Description : Game lobby for daemon-games
 Author      : Maurits van der Schee <maurits@vdschee.nl>
 URL         : https://github.com/mevdschee/daemon-games
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/select.h>

#include "daemon.h"
#include "lobby.h"

typedef struct lobby_t lobby_t;

struct lobby_t {
	daemon_t *daemon;
	void (*start_game)(daemon_t *daemon);
};

lobby_t *lobby_create()
{
	lobby_t *lobby;
	lobby = malloc(sizeof(*lobby));
	memset(lobby,0,sizeof(*lobby));
	return lobby;
}

void lobby_destroy(lobby_t *lobby)
{
	free(lobby);
}

void lobby_on_tick(daemon_t *daemon, int tick)
{
	fprintf(stdout,".");
	fflush(stdout);
}

void lobby_on_data(daemon_t *daemon, int client)
{
	int nbytes;
	char bytes[1024];

	fprintf(stdout,"#%d",client);
	fflush(stdout);

	nbytes = daemon_read(daemon, client, bytes, sizeof(bytes));
	if (nbytes) {
		daemon_write(daemon, client, bytes, nbytes);
	}
}

void lobby_on_connect(daemon_t *daemon, int client)
{
	int nbytes;
	char *bytes;

	uint8_t *addr = (uint8_t *)&daemon->client_address[client].sin_addr.s_addr;
	uint16_t *port = (uint16_t *)&daemon->client_address[client].sin_port;

	fprintf(stdout,"#%d[%d.%d.%d.%d:%d]",client,addr[0],addr[1],addr[2],addr[3],*port);
	fflush(stdout);

	bytes = "Welcome\n";
	nbytes = strlen(bytes);

	daemon_write(daemon,client,bytes,nbytes);

	lobby_t *lobby = (lobby_t *)daemon->context;

	lobby->start_game(daemon);
	lobby_destroy(lobby);
}

void lobby_on_disconnect(daemon_t *daemon, int client)
{
	fprintf(stdout,"[disconnect]");
	fflush(stdout);
}

void lobby_run(daemon_t *daemon, void (*start_game)(daemon_t *daemon))
{
	lobby_t *lobby;
	lobby = lobby_create();
	lobby->daemon = daemon;
	lobby->start_game = start_game;
	daemon->context = (void *)lobby;

	daemon->on_connect = lobby_on_connect;
	daemon->on_disconnect = lobby_on_disconnect;
	daemon->on_data = lobby_on_data;
	daemon->on_tick = lobby_on_tick;
}
