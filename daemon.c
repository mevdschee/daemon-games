/*
 ============================================================================
 Name        : daemon.c
 Description : Simple single threaded TCP daemon
 Author      : Maurits van der Schee <maurits@vdschee.nl>
 URL         : https://github.com/mevdschee/daemon_games
 ============================================================================
 */
#define _XOPEN_SOURCE 500
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

void daemon_disconnect(daemon_t *daemon, int client)
{
	close(daemon->client_fd[client]);
	daemon->client_fd[client] = -1;
	daemon->on_disconnect(daemon, client);
}

int daemon_read(daemon_t *daemon, int client, char *bytes, int nbytes)
{
	int result;

	result = read(daemon->client_fd[client], bytes, nbytes);
	if (result == 0) {
		daemon_disconnect(daemon,client);
	} else if (result < 0) {
		if (errno == ECONNABORTED) {
			daemon_disconnect(daemon,client);
		}
		fprintf(stderr, "read failed\n");
		daemon_disconnect(daemon,client);
	}
	return result;
}

void daemon_write(daemon_t *daemon, int client, char *bytes, int nbytes)
{
	if (write(daemon->client_fd[client], bytes, nbytes) < nbytes) {
		fprintf(stderr, "write failed\n");
		daemon_disconnect(daemon,client);
	}
}

bool daemon_listen(daemon_t *daemon)
{
	int value;

	daemon->server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (daemon->server_fd < 0) {
		fprintf(stderr, "Could not create socket\n");
		return false;
	}

	value = 1;
	if (setsockopt(daemon->server_fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)) < 0) {
		fprintf(stderr, "Could not set socket reuse option\n");
		return false;
	}

	memset(&daemon->server_address, 0 ,sizeof(daemon->server_address));
	daemon->server_address.sin_family = AF_INET;
	daemon->server_address.sin_port = htons(daemon->port);
	daemon->server_address.sin_addr.s_addr = htonl(daemon->ip);

	if (bind(daemon->server_fd, (const struct sockaddr *)&daemon->server_address, sizeof(daemon->server_address)) < 0) {
		fprintf(stderr, "Could not bind socket\n");
		return false;
	}

	if (listen(daemon->server_fd, 0) < 0){ // 1 or daemon->slots?
		fprintf(stderr, "Could not listen on socket\n");
		return false;
	}

	return true;
}

void daemon_set_timeout(daemon_t *daemon, struct timeval *timeout)
{
	struct timeval now;
	long diff, usec;

	timeout->tv_sec = 0;
	gettimeofday(&now,NULL);
	diff = (now.tv_sec-daemon->lasttick.tv_sec) * 1000000L;
	diff = diff + (now.tv_usec - daemon->lasttick.tv_usec);
	usec = 1000000L/daemon->ticks - diff;
	if (usec<0) {
		fprintf(stderr, "Could not reach tick rate\n");
		usec = 0;
	}
	timeout->tv_usec = usec;

}

void daemon_destroy(daemon_t *daemon)
{
	free(daemon->client_address);
	free(daemon->client_fd);
	free(daemon);
}

bool daemon_run(daemon_t *daemon)
{
	int i,ready,len,nclients,maxfd,tick;
	bool success;
	struct timeval timeout;

	for (i=0;i<daemon->slots;i++) {
		daemon->client_fd[i] = -1;
	}

	success = daemon_listen(daemon);
	gettimeofday(&daemon->lasttick,NULL);
	tick = 0;

	while (success) {
		FD_ZERO(&daemon->fds);
		FD_SET(daemon->server_fd, &daemon->fds);
		maxfd = daemon->server_fd;
		nclients = 1;
		for (i=0;i<daemon->slots;i++) {
			if (daemon->client_fd[i] >= 0) {
				FD_SET(daemon->client_fd[i], &daemon->fds);
				maxfd = daemon->client_fd[i]>maxfd?daemon->client_fd[i]:maxfd;
				nclients++;
			}
		}

		daemon_set_timeout(daemon, &timeout);
		ready = select(maxfd+1, &daemon->fds, NULL, NULL, &timeout);
		if (ready == 0) {
			gettimeofday(&daemon->lasttick,NULL);
			daemon->on_tick(daemon,tick);
			tick = (tick+1) % daemon->ticks;
		}
		if (ready < 0) {
			fprintf(stderr, "Could not select from sockets\n");
			success = false;
			break;
		}
		// if server has data
		if ((daemon->server_fd >= 0) && FD_ISSET(daemon->server_fd, &daemon->fds)) {

			// new connection
			for (i=0;i<daemon->slots;i++) {
				if (daemon->client_fd[i] < 0) {

					memset(&daemon->client_address[i], 0 ,sizeof(daemon->client_address[i]));
					len = sizeof(daemon->client_address[i]);
					daemon->client_fd[i] = accept(daemon->server_fd, (struct sockaddr *)&daemon->client_address[i], &len);
					if (daemon->client_fd[i] < 0) {
						fprintf(stderr, "accept failed\n");
						break;
					}
					daemon->on_connect(daemon,i);
					break;
				}
			}
			if (i==daemon->slots) {
				close(accept(daemon->server_fd, NULL, NULL));
				fprintf(stderr, "client denied, max clients reached\n");
			}
		}
		/* check all connections */
		for (i=0;i<daemon->slots;i++) {
			// if client has data
			if ((daemon->client_fd[i] >= 0) && FD_ISSET(daemon->client_fd[i], &daemon->fds)) {
				daemon->on_data(daemon,i);
			}
		}
	}

	for (i=0;i<daemon->slots;i++) {
		if (daemon->client_fd[i] >= 0) {
			close(daemon->client_fd[i]);
		}
	}
	close(daemon->server_fd);
	daemon_destroy(daemon);

	return success;
}

void daemon_on_tick(daemon_t *daemon, int tick)
{
	fprintf(stdout,".");
	fflush(stdout);
}

void daemon_on_data(daemon_t *daemon, int client)
{
	int nbytes;
	char bytes[1024];

	fprintf(stdout,"#%d",client);
	fflush(stdout);

	nbytes = daemon->read(daemon, client, bytes, sizeof(bytes));
	if (nbytes) {
		daemon->write(daemon, client, bytes, nbytes);
	}
}

void daemon_on_connect(daemon_t *daemon, int client)
{
	int nbytes;
	char *bytes;

	uint8_t *addr = (uint8_t *)&daemon->client_address[client].sin_addr.s_addr;
	uint16_t *port = (uint16_t *)&daemon->client_address[client].sin_port;

	fprintf(stdout,"#%d[%d.%d.%d.%d:%d]",client,addr[0],addr[1],addr[2],addr[3],*port);
	fflush(stdout);

	bytes = "Welcome\n";
	nbytes = strlen(bytes);

	daemon->write(daemon,client,bytes,nbytes);
}

void daemon_on_disconnect(daemon_t *daemon, int client)
{
	fprintf(stdout,"[disconnect]");
	fflush(stdout);
}

daemon_t *daemon_create(uint32_t ip, uint16_t port, uint16_t slots, uint8_t ticks)
{
	daemon_t *daemon;
	int i;

	daemon = malloc(sizeof(*daemon));
	memset(daemon,0,sizeof(*daemon));
	// initialization values
	daemon->ip = ip;
	daemon->port = port;
	daemon->slots = slots;
	daemon->ticks = ticks;
	// public variables
	memset(&daemon->server_address,0,sizeof(daemon->server_address));
	daemon->client_address = malloc(slots * sizeof(*daemon->client_address));
	memset(daemon->client_address,0,slots * sizeof(*daemon->client_address));
	// private variables
	daemon->server_fd = -1;
	daemon->client_fd = malloc(slots * sizeof(*daemon->client_fd));
	for (i=0;i<slots;i++) {
		daemon->client_fd[i] = -1;
	}
	// public functions
	daemon->disconnect = daemon_disconnect;
	daemon->read = daemon_read;
	daemon->write = daemon_write;
	// event handlers
	daemon->on_tick = daemon_on_tick;
	daemon->on_data = daemon_on_data;
	daemon->on_connect = daemon_on_connect;
	daemon->on_disconnect = daemon_on_disconnect;
	return daemon;
}
