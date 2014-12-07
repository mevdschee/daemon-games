/*
 ============================================================================
 Name        : lobby.h
 Description : Game lobby for daemon-games
 Author      : Maurits van der Schee <maurits@vdschee.nl>
 URL         : https://github.com/mevdschee/daemon-games
 ============================================================================
 */

#ifndef LOBBY_H_
#define LOBBY_H_

#include "daemon.h"

void lobby_run(daemon_t *daemon, void (*start_game)(daemon_t *daemon));

#endif /* LOBBY_H_ */
