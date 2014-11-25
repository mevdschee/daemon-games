#ifndef STRBUF_H_
#define STRBUF_H_

typedef struct {
	char *buffer;
	size_t size;
} strbuf_t;

strbuf_t *strbuf_create();

void strbuf_destroy(strbuf_t *sb);

int strbuf_append(strbuf_t *sb, const char *fmt, ...);

int strbuf_set(strbuf_t *sb, const char *fmt, ...);

#endif /* STRBUF_H_ */
