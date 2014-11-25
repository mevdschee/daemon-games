/*
 ============================================================================
 Name        : snaked.c
 Description : Multi-user console snake game for GNU/Linux
 Author      : Maurits van der Schee <maurits@vdschee.nl>
 URL         : https://github.com/mevdschee/daemon-games
 ============================================================================
 */
#define _XOPEN_SOURCE 500
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "daemon.h"
#include "strbuf.h"

typedef struct snake_t snake_t;

enum directions { down, up, right, left };
enum fields { current, previous, directions };

struct snake_position_t {
	int x,y;
};

struct snake_player_t {
	int number;
	struct snake_position_t head, tail;
};

struct snake_t {
	struct snake_player_t *players;
	int width;
	int height;
	int *field;
};

int snake_get(snake_t *snake, int field, int x, int y)
{
	return *(snake->field + field*snake->width*snake->height + y*snake->width + x);
}

void snake_set(snake_t *snake, int field, int x, int y, int value)
{
	*(snake->field + field*snake->width*snake->height + y*snake->width + x) = value;
}

snake_t *snake_create(int width, int height, int slots)
{
	snake_t *snake = malloc(sizeof(*snake));
	snake->players = malloc(slots*sizeof(*snake->players));
	snake->width = width;
	snake->height = height;
	size_t size = 3*snake->width*snake->height*sizeof(*snake->field);
	snake->field = malloc(size);
	memset(snake->field,0,size);
	return snake;
}

void snake_destroy(snake_t *snake)
{
	free(snake->field);
	free(snake->players);
	free(snake);
}

void on_tick(daemon_t *daemon, int tick)
{
	snake_t *snake = (snake_t *)daemon->context;

	int i,x,y,c,p;
	int cx=-1,cy=-1;
	strbuf_t *sb = strbuf_create();

	for (y=0;y<snake->height;y++) {
		for (x=0;x<snake->width;x++) {
			c=snake_get(snake,current,x,y);
			p=snake_get(snake,previous,x,y);
			if (c!=p || 1) {
				if (cx!=x || cy!=y) { //move cursor
					strbuf_append(sb,"\e[%d;%dH",y,x*2);
				}
				// draw and increment cx
				switch(c) {
					/*case 0:  strbuf_append(sb,"\e[30;40m  "); break;
					case 1:  strbuf_append(sb,"\e[31;40m<>"); break;
					case 10: strbuf_append(sb,"\e[30;41mhh"); break;
					case 11: strbuf_append(sb,"\e[30;41mbb"); break;
					case 12: strbuf_append(sb,"\e[30;41mtt"); break;
					case 21: strbuf_append(sb,"\e[30;41mhh"); break;
					case 22: strbuf_append(sb,"\e[30;41mbb"); break;
					case 23: strbuf_append(sb,"\e[30;41mtt"); break;*/
					default: strbuf_append(sb,"  "); break;
				}
				cx++;
				int value = snake_get(snake,current,x,y);
				snake_set(snake,previous,x,y,value);
			}
		}
	}

	for(i=0;i<daemon->slots;i++) {
		if (daemon->client_fd[i]==-1) continue;
		daemon_write(daemon,i,sb->buffer,strlen(sb->buffer)+1);
	}

	strbuf_destroy(sb);
}

void on_data(daemon_t *daemon, int client)
{
	snake_t *snake = (snake_t *)daemon->context;

	int nbytes;
	char bytes[128];

	nbytes = daemon_read(daemon, client, bytes, sizeof(bytes));
	if (nbytes) {
		if (bytes[0]=='q') {
			daemon_disconnect(daemon, client);
		} else {
			daemon_write(daemon, client, bytes, nbytes);
		}
	}
}

void on_connect(daemon_t *daemon, int client)
{
	snake_t *snake = (snake_t *)daemon->context;

	int nbytes;
	char *bytes;

	bytes = "\e[30m\e[41m''\e[42m: \e[42m  \e[0m\e[30m\n\e[41m  \e[41m  \e[42m  \e[0m";
	nbytes = strlen(bytes);

	daemon_write(daemon,client,bytes,nbytes);
}

void on_disconnect(daemon_t *daemon, int client)
{
	snake_t *snake = (snake_t *)daemon->context;

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

	int ip = 0, slots = 2, ticks = 10, width = 40, height = 20;



	daemon_t *daemon = daemon_create(ip, port, slots, ticks);
	snake_t *snake = snake_create(width, height, slots);

	daemon->context = (void *)snake;

	daemon->on_connect = on_connect;
	daemon->on_disconnect = on_disconnect;
	daemon->on_data = on_data;
	daemon->on_tick = on_tick;

	return daemon_run(daemon)?EXIT_SUCCESS:EXIT_FAILURE;
}
