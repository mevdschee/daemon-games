#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "strbuf.h"

strbuf_t *strbuf_create()
{
	strbuf_t *sb = malloc(sizeof(strbuf_t));
	sb->size = 1024;
	sb->buffer = malloc(sb->size);
	memset(sb->buffer,0,sb->size);
	return sb;
}

void strbuf_destroy(strbuf_t *sb)
{
	free(sb->buffer);
	free(sb);
}

int strbuf_append_str(strbuf_t *sb, char *str, size_t len)
{
	size_t pos = strlen(sb->buffer);
	if (pos>=sb->size) {
		return -1;
	}
	if (sb->size-pos<len+1) {
		char *new = realloc(sb->buffer, sb->size*2);
		if (new == NULL) {
			return -1;
		}
		memcpy(new, sb->buffer, sb->size);
		memset(new+sb->size, 0, sb->size);
		sb->buffer = new;
		sb->size *= 2;
	}
	memcpy(sb->buffer+pos,str,len+1);
	return len;
}

int strbuf_vappend(strbuf_t *sb, const char *fmt, va_list ap)
{
    int n;
    int size = 1024; /* Guess we need no more than 1024 bytes */
    char *p, *np;

    p = malloc(size);
    if (p == NULL) {
    	return -1;
    }

    while (1) {

        /* Try to print in the allocated space */

        n = vsnprintf(p, size, fmt, ap);

        /* Check error code */

        if (n < 0) {
            free(p);
            return -1;
        }

        /* If that worked, return the string */

        if (n < size) {
        	int res = strbuf_append_str(sb,p,n);
        	free(p);
        	return res;
        }

        /* Else try again with more space */

        size = n + 1;       /* Precisely what is needed */

        np = realloc(p, size);
        if (np == NULL) {
            free(p);
            return -1;
        } else {
            p = np;
        }
    }
    return -1;
}

int strbuf_append(strbuf_t *sb, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int res = strbuf_vappend(sb,fmt,ap);
    va_end(ap);
    return res;
}

int strbuf_set(strbuf_t *sb, const char *fmt, ...)
{
	sb->buffer[0]=0;
	va_list ap;
    va_start(ap, fmt);
    int res = strbuf_vappend(sb,fmt,ap);
    va_end(ap);
    return res;
}
