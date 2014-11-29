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
	int px,py;
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
	size_t size = 3*width*height*sizeof(*snake->field);
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

void snake_move_tails(snake_t *snake)
{
	int player;
	struct snake_position_t head, tail;

	// move tail out of the way (if not scored)
	for (player=0;player<snake->nplayers;player++) {
		if (snake->players[player].alive) {
			head = snake->players[player].head;
			tail = snake->players[player].tail;
			// did we move?
			if (snake_get(snake,current,head.x,head.y)==0) {
				// update field
				snake_set(snake,directions,tail.px,tail.py,0);
				snake_set(snake,current,tail.px,tail.py,0);
				snake_set(snake,current,tail.x,tail.y,12+player*10);
			}
		}
	}
}

void snake_move_heads(snake_t *snake)
{
	int player;
	char direction;
	struct snake_position_t head;

	// try to move head
	for (player=0;player<snake->nplayers;player++) {
		if (snake->players[player].alive) {
			head = snake->players[player].head;
			direction = snake->players[player].direction;
			// if we hit something not that can be eaten
			if (snake_get(snake,current,head.x,head.y)<10) {
				snake_set(snake,directions,head.px,head.py,direction);
				snake_set(snake,directions,head.x,head.y,direction);
				if (snake->players[player].length>2) {
					snake_set(snake,current,head.px,head.py,11+player*10);
				}
				snake_set(snake,current,head.x,head.y,10+player*10);
			} else {
				// you have hit something that kills you
				snake->players[player].alive = false;
			}
		}
	}
}

void snake_update_coordinate(snake_t *snake, struct snake_position_t *p, int w, int h, char direction)
{
	p->px = p->x;
	p->py = p->y;

	switch (direction) {
		case down: p->y=(p->py+1)%h;   break;
		case up:   p->y=(p->py+h-1)%h; break;
		case right:p->x=(p->px+1)%w;   break;
		case left: p->x=(p->px+w-1)%w; break;
	}
}

void snake_update_coordinates(snake_t *snake)
{
	int player,width,height;
	char tail_direction,head_direction;

	width = snake->width;
	height = snake->height;

	// update head and tail coordinates
	for (player=0;player<snake->nplayers;player++) {
		if (snake->players[player].alive) {
			tail_direction = snake_get(snake,directions,snake->players[player].tail.x,snake->players[player].tail.y);
			head_direction = snake->players[player].direction;
			snake_update_coordinate(snake,&snake->players[player].tail,width,height,tail_direction);
			snake_update_coordinate(snake,&snake->players[player].head,width,height,head_direction);
		}
	}
}

void snake_next_frame(snake_t *snake)
{
	size_t field_size;

	field_size = snake->width*snake->height*sizeof(*snake->field);
	memcpy(snake->field+previous*field_size,snake->field+current*field_size,field_size);

	snake_update_coordinates(snake);
	snake_move_tails(snake);
	snake_move_heads(snake);
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

	int i,nbytes;
	char direction;
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
		break;
	}
}

void on_connect(daemon_t *daemon, int client)
{
	snake_t *snake = (snake_t *)daemon->context;

	if (!snake->players[client].length) {
		snake->players[client].alive = true;
		snake->players[client].head.x = 0;
		snake->players[client].head.y = 1;
		snake->players[client].head.px = 0;
		snake->players[client].head.py = 1;
		snake->players[client].tail.x = 0;
		snake->players[client].tail.y = 0;
		snake->players[client].tail.px = 0;
		snake->players[client].tail.py = 0;
		snake->players[client].length = 2;
		snake->players[client].direction = down;
		snake_set(snake,directions,snake->players[client].head.x,snake->players[client].head.y,down);
		snake_set(snake,directions,snake->players[client].tail.x,snake->players[client].tail.y,down);
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
