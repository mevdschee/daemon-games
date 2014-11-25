/*
 ============================================================================
 Name        : daemon.h
 Description : Simple single threaded TCP daemon
 Author      : Maurits van der Schee <maurits@vdschee.nl>
 URL         : https://github.com/mevdschee/daemon-games
 ============================================================================
 */

#ifndef DAEMON_H_
#define DAEMON_H_

#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>
#include <arpa/inet.h>

typedef struct daemon_t daemon_t;

struct daemon_t {
	// initialization values
	uint32_t ip;
	uint16_t port;
	uint16_t slots;
	uint8_t ticks;
	// public variables
	void *context;
	struct sockaddr_in server_address;
	struct sockaddr_in *client_address;
	// private variables
	int server_fd;
	int *client_fd;
	fd_set fds;
	struct timeval lasttick;
	// public functions
	void (*disconnect)(daemon_t *daemon, int client);
	int (*read)(daemon_t *daemon, int client, char *bytes, int nbytes);
	void (*write)(daemon_t *daemon, int client, char *bytes, int nbytes);
	// event handlers
	void (*on_connect)(daemon_t *daemon, int client);
	void (*on_disconnect)(daemon_t *daemon, int client);
	void (*on_data)(daemon_t *daemon, int client);
	void (*on_tick)(daemon_t *daemon, int tick);
};

daemon_t *daemon_create(uint32_t ip, uint16_t port, uint16_t slots, uint8_t ticks);
bool daemon_run(daemon_t *daemon);

void daemon_disconnect(daemon_t *daemon, int client);
int daemon_read(daemon_t *daemon, int client, char *bytes, int nbytes);
int daemon_write(daemon_t *daemon, int client, char *bytes, int nbytes);

#endif /* DAEMON_H_ */
