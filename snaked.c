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
	bool alive;
	int length;
	char direction;
	struct snake_position_t head, tail;
};

struct snake_t {
	int nplayers;
	struct snake_player_t *players;
	int width;
	int height;
	char *field;
};

char snake_get(snake_t *snake, int field, int x, int y)
{
	return *(snake->field + field*snake->width*snake->height + y*snake->width + x);
}

void snake_set(snake_t *snake, int field, int x, int y, char c)
{
	*(snake->field + field*snake->width*snake->height + y*snake->width + x) = c;
}

snake_t *snake_create(int width, int height, int slots)
{
	snake_t *snake = malloc(sizeof(*snake));
	snake->nplayers = slots;
	snake->players = malloc(snake->nplayers*sizeof(*snake->players));
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

void snake_next_frame(snake_t *snake)
{
	int x,y,w,h,i;
	char c;

	// memcpy?
	for (y=0;y<snake->height;y++) {
		for (x=0;x<snake->width;x++) {
			c = snake_get(snake,current,x,y);
			snake_set(snake,previous,x,y,c);
		}
	}

	w = snake->width;
	h = snake->height;

	// move tail out of the way (if not scored)
	for (i=0;i<snake->nplayers;i++) {
		if (snake->players[i].alive) {
			x = snake->players[i].tail.x;
			y = snake->players[i].tail.y;
			if (snake->players[i].length>1) {
				snake_set(snake,current,x,y,0);
			}
			switch (snake_get(snake,directions,x,y)) {
				case down: y=(y+1)%h;   break;
				case up:   y=(y+h-1)%h; break;
				case right:x=(x+1)%w;   break;
				case left: x=(x+w-1)%w; break;
			}
			if (snake->players[i].length>1) {
				snake_set(snake,current,x,y,12+i*10);
			}
			snake->players[i].tail.x = x;
			snake->players[i].tail.y = y;
		}
	}

	// try to move head
	for (i=0;i<snake->nplayers;i++) {
		if (snake->players[i].alive) {
			x = snake->players[i].head.x;
			y = snake->players[i].head.y;
			if (snake->players[i].length>1) {
				if (snake->players[i].length>2) {
					snake_set(snake,current,x,y,11+i*10);
				}
				snake_set(snake,directions,x,y,snake->players[i].direction);
			} else if (snake->players[i].length==1) {
				snake_set(snake,current,x,y,0);
			}
			switch (snake->players[i].direction) {
				case down: y=(y+1)%h;   break;
				case up:   y=(y+h-1)%h; break;
				case right:x=(x+1)%w;   break;
				case left: x=(x+w-1)%w; break;
			}
			if (snake_get(snake,current,x,y)==0) {
				snake_set(snake,directions,x,y,snake->players[i].direction);
				snake_set(snake,current,x,y,10+i*10);
			} else {
				// you have hit something
				snake->players[i].alive = false;
			}
			snake->players[i].head.x = x;
			snake->players[i].head.y = y;
		}
	}

}

void snake_get_frame(snake_t *snake, strbuf_t *sb, bool full)
{
	int x,y,c,p;
	int cx=-1,cy=-1;

	if (full) {
		strbuf_append(sb,"\e[?25l\e[2J");
	}

	for (y=0;y<snake->height;y++) {
		for (x=0;x<snake->width;x++) {
			c=snake_get(snake,current,x,y);
			p=snake_get(snake,previous,x,y);
			if (c!=p || full) {
				if (cx!=x || cy!=y) { //move cursor
					strbuf_append(sb,"\e[%d;%dH",y+1,x*2+1);
				}
				// draw and increment cx
				switch(c) {
					case 0:  strbuf_append(sb,"\e[30;40m  "); break;
					case 1:  strbuf_append(sb,"\e[31;40m<>"); break;
					case 10: strbuf_append(sb,"\e[30;41mhh"); break;
					case 11: strbuf_append(sb,"\e[30;41mbb"); break;
					case 12: strbuf_append(sb,"\e[30;41mtt"); break;
					case 20: strbuf_append(sb,"\e[30;42mhh"); break;
					case 21: strbuf_append(sb,"\e[30;42mbb"); break;
					case 22: strbuf_append(sb,"\e[30;42mtt"); break;
				}
				cx++;
			}
		}
	}

}

void on_tick(daemon_t *daemon, int tick)
{
	int i;

	snake_t *snake = (snake_t *)daemon->context;

	strbuf_t *sb = strbuf_create();

	snake_next_frame(snake);

	snake_get_frame(snake, sb, false);

	for(i=0;i<daemon->slots;i++) {
		if (daemon->client_fd[i]==-1) continue;
		daemon_write(daemon,i,sb->buffer,strlen(sb->buffer)+1);
	}

	strbuf_destroy(sb);
}

void on_data(daemon_t *daemon, int client)
{
	snake_t *snake = (snake_t *)daemon->context;

	int i,nbytes, direction;
	char bytes[2048];

	nbytes = daemon_read(daemon, client, bytes, sizeof(bytes));
	for (i=0;i<nbytes;i++) {
		direction = snake_get(snake,directions,snake->players[client].head.x,snake->players[client].head.y);
		switch (bytes[i]) {
			case 'q': daemon_disconnect(daemon, client); break;
			case 'w': if (direction!=down)  snake->players[client].direction = up;    break;
			case 'a': if (direction!=right) snake->players[client].direction = left;  break;
			case 's': if (direction!=up)    snake->players[client].direction = down;  break;
			case 'd': if (direction!=left)  snake->players[client].direction = right; break;
		}
	}
}

void on_connect(daemon_t *daemon, int client)
{
	snake_t *snake = (snake_t *)daemon->context;

	if (!snake->players[client].length) {
		snake->players[client].alive = true;
		snake->players[client].head.x = 0;
		snake->players[client].head.y = 0;
		snake->players[client].tail.x = 0;
		snake->players[client].tail.y = 0;
		snake->players[client].length = 1;
		snake->players[client].direction = down;
		snake_set(snake,directions,snake->players[client].head.x,snake->players[client].head.y,down);
	}

	strbuf_t *sb = strbuf_create();

	snake_get_frame(snake, sb, true);

	daemon_write(daemon,client,sb->buffer,strlen(sb->buffer)+1);

	strbuf_destroy(sb);
}

void on_disconnect(daemon_t *daemon, int client)
{
	snake_t *snake = (snake_t *)daemon->context;

	snake->players[client].alive = false;
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

	int ip = 0, slots = 2, ticks = 4, width = 40, height = 20;



	daemon_t *daemon = daemon_create(ip, port, slots, ticks);
	snake_t *snake = snake_create(width, height, slots);

	daemon->context = (void *)snake;

	daemon->on_connect = on_connect;
	daemon->on_disconnect = on_disconnect;
	daemon->on_data = on_data;
	daemon->on_tick = on_tick;

	return daemon_run(daemon)?EXIT_SUCCESS:EXIT_FAILURE;
}
