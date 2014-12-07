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
#include "lobby.h"
#include "strbuf.h"

typedef struct snake_t snake_t;

enum directions { none, down, up, right, left };

struct snake_position_t {
	int x,y;
};

struct snake_player_t {
	bool alive;
	int length;
	char direction;
	struct snake_position_t head, tail, previous_head, previous_tail;
};

struct snake_t {
	int nplayers;
	struct snake_player_t *players;
	int width;
	int height;
	char *fields;
	char *previous_fields;
	char *directions;
};

char snake_get(char *field, int w, int h, int x, int y)
{
	if (x < 0 || y < 0 || x >= w || y >= h) {
		fprintf(stderr, "Read from field out of bounds\n");
		exit(EXIT_FAILURE);
	}

	return *(field + (y * w + x) * sizeof(*field));
}

void snake_set(char *field, int w, int h, int x, int y, char c)
{
	if (x < 0 || y < 0 || x >= w || y >= h) {
		fprintf(stderr, "Write to field out of bounds\n");
		exit(EXIT_FAILURE);
	}
	*(field + (y * w + x) * sizeof(*field)) = c;
}

char snake_get_field(snake_t *snake, struct snake_position_t *pos)
{
	return snake_get(snake->fields,snake->width,snake->height,pos->x,pos->y);
}

char snake_get_direction(snake_t *snake, struct snake_position_t *pos)
{
	return snake_get(snake->directions,snake->width,snake->height,pos->x,pos->y);
}

void snake_set_field(snake_t *snake, struct snake_position_t *pos, char c)
{
	snake_set(snake->fields,snake->width,snake->height,pos->x,pos->y,c);
}

void snake_set_direction(snake_t *snake, struct snake_position_t *pos, char c)
{
	snake_set(snake->directions,snake->width,snake->height,pos->x,pos->y,c);
}

snake_t *snake_create(int width, int height, int slots)
{
	snake_t *snake = malloc(sizeof(*snake));
	snake->nplayers = slots;
	snake->players = malloc(snake->nplayers*sizeof(*snake->players));
	snake->width = width;
	snake->height = height;
	size_t field_size = width*height*sizeof(*snake->fields);
	snake->fields = malloc(field_size);
	memset(snake->fields,0,field_size);
	snake->previous_fields = malloc(field_size);
	memset(snake->previous_fields,0,field_size);
	snake->directions = malloc(field_size);
	memset(snake->directions,none,field_size);
	return snake;
}

void snake_destroy(snake_t *snake)
{
	free(snake->fields);
	free(snake->previous_fields);
	free(snake->directions);
	free(snake->players);
	free(snake);
}

void snake_move_tails(snake_t *snake)
{
	int player;
	struct snake_position_t *head, *tail, *previous_head, *previous_tail;

	// move tail out of the way (if not scored)
	for (player=0;player<snake->nplayers;player++) {
		if (snake->players[player].alive) {
			head = &snake->players[player].head;
			tail = &snake->players[player].tail;
			previous_head = &snake->players[player].previous_head;
			previous_tail = &snake->players[player].previous_tail;
			// did we move?
			if (snake_get_field(snake,head)==0) {
				// update field
				snake_set_direction(snake,previous_tail,none);
				snake_set_field(snake,previous_tail,0);
				snake_set_field(snake,tail,12+player*10);
			} else if (snake_get_field(snake,head)==1) {
				// food, increase length, no tail move
				snake->players[player].length++;
				*tail = *previous_tail;
			}
		}
	}
}

void snake_move_heads(snake_t *snake)
{
	int player;
	char direction;
	struct snake_position_t *head, *previous_head;

	// try to move head
	for (player=0;player<snake->nplayers;player++) {
		if (snake->players[player].alive) {
			head = &snake->players[player].head;
			previous_head = &snake->players[player].previous_head;
			direction = snake->players[player].direction;
			// if we hit something that can be eaten
			if (snake_get_field(snake,head)<10) {
				snake_set_direction(snake,previous_head,direction);
				snake_set_direction(snake,head,direction);
				if (snake->players[player].length>2) {
					snake_set_field(snake,previous_head,11+player*10);
				}
				snake_set_field(snake,head,10+player*10);
			} else {
				// you have hit something that kills you
				snake->players[player].alive = false;
			}
		}
	}
}

void snake_update_coordinate(snake_t *snake, struct snake_position_t *pp, struct snake_position_t *p, int w, int h, char direction)
{
	pp->x = p->x;
	pp->y = p->y;

	switch (direction) {
		case down: p->y=(pp->y+1)%h;   break;
		case up:   p->y=(pp->y+h-1)%h; break;
		case right:p->x=(pp->x+1)%w;   break;
		case left: p->x=(pp->x+w-1)%w; break;
	}
}

void snake_update_coordinates(snake_t *snake)
{
	int player, width, height;
	char tail_direction, head_direction;
	struct snake_position_t *head, *tail, *previous_head, *previous_tail;

	width = snake->width;
	height = snake->height;

	// update head and tail coordinates
	for (player=0;player<snake->nplayers;player++) {
		if (snake->players[player].alive) {
			tail_direction = snake_get_direction(snake, &snake->players[player].tail);
			head_direction = snake->players[player].direction;
			head = &snake->players[player].head;
			tail = &snake->players[player].tail;
			previous_head = &snake->players[player].previous_head;
			previous_tail = &snake->players[player].previous_tail;
			snake_update_coordinate(snake, previous_tail, tail, width, height, tail_direction);
			snake_update_coordinate(snake, previous_head, head, width, height, head_direction);
		}
	}
}

void snake_next_frame(snake_t *snake)
{
	size_t field_size;

	field_size = snake->width*snake->height*sizeof(*snake->fields);
	memcpy(snake->previous_fields,snake->fields,field_size);

	snake_update_coordinates(snake);
	snake_move_tails(snake);
	snake_move_heads(snake);
}

void snake_get_frame(snake_t *snake, strbuf_t *sb, bool full)
{
	int x, y, cx, cy;
	char c, d, p;

	if (full) {
		strbuf_append(sb,"\e[?25l\e[2J");
	}
	strbuf_append(sb,"\e[H");

	// none, down, up, right, left
	char heads[] = "  ..'' :: ";

	cx = cy = 0;
	for (y=0;y<snake->height;y++) {
		for (x=0;x<snake->width;x++) {
			c=snake_get(snake->fields,snake->width,snake->height,x,y);
			p=snake_get(snake->previous_fields,snake->width,snake->height,x,y);
			if (c!=p || full) {
				if (x==0 && y==cy+1) { // new line?
					strbuf_append(sb,"\n");
				} else if (cx!=x || cy!=y) { // move cursor
					strbuf_append(sb,"\e[%d;%dH",y+1,x*2+1);
				}
				// get direction
				d=snake_get(snake->directions,snake->width,snake->height,x,y);
				// draw and increment cx
				switch(c) {
					case 0:  strbuf_append(sb,"\e[0;30;40m  "); break;
					case 1:  strbuf_append(sb,"\e[1;37;40m<>"); break;
					case 10: strbuf_append(sb,"\e[0;30;41m%c%c",heads[d*2],heads[d*2+1]); break;
					case 11: strbuf_append(sb,"\e[0;30;41m  "); break;
					case 12: strbuf_append(sb,"\e[0;30;41m  "); break;
					case 20: strbuf_append(sb,"\e[0;30;42m%c%c",heads[d*2],heads[d*2+1]); break;
					case 21: strbuf_append(sb,"\e[0;30;42m  "); break;
					case 22: strbuf_append(sb,"\e[0;30;42m  "); break;
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

	if (tick%100==0) {
		int x = rand()%snake->width;
		int y = rand()%snake->height;
		if (snake_get(snake->fields,snake->width,snake->height,x,y)==0) {
			snake_set(snake->fields,snake->width,snake->height,x,y,1);
		}
	}

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
		direction = snake_get_direction(snake, &snake->players[client].head);
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

	int x = client * snake->width / daemon->slots;

	if (!snake->players[client].length) {
		snake->players[client].alive = true;
		snake->players[client].head.x = x;
		snake->players[client].head.y = 1;
		snake->players[client].previous_head.x = x;
		snake->players[client].previous_head.y = 1;
		snake->players[client].tail.x = x;
		snake->players[client].tail.y = 0;
		snake->players[client].previous_tail.x = x;
		snake->players[client].previous_tail.y = 0;
		snake->players[client].length = 2;
		snake->players[client].direction = down;
		snake_set_direction(snake, &snake->players[client].head, down);
		snake_set_direction(snake, &snake->players[client].tail, down);
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

void snake_start_game(daemon_t *daemon)
{
	int i, width = 40, height = 20;

	snake_t *snake = snake_create(width, height, daemon->slots);
	daemon->context = (void *)snake;

    // initialize users from lobby

	for (i=0;i<daemon->slots;i++) {
		if (daemon->client_fd[i] >= 0) {
			on_connect(daemon,i);
		}
	}

	daemon->on_connect = on_connect;
	daemon->on_disconnect = on_disconnect;
	daemon->on_data = on_data;
	daemon->on_tick = on_tick;
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
	lobby_run(daemon, snake_start_game);

	return daemon_run(daemon)?EXIT_SUCCESS:EXIT_FAILURE;
}
