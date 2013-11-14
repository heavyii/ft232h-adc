#include <sys/param.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#ifdef __linux__
#include <sys/param.h>
#else /* __linux__ */
/* Macros for min/max.  */
#define		MIN(a,b) (((a)<(b))?(a):(b))
#define		MAX(a,b) (((a)>(b))?(a):(b))
#define 	roundup(x, y)	((((x) + ((y) - 1)) / (y)) * (y))
#endif /* __linux__ */

#include "buffer.h"

#define		BUFFER_MAX_CHUNK	0x100000	/** 1Mb  */
#define		BUFFER_MAX_LEN		0x4000000	/** 64MB */
#define		BUFFER_ALLOCSZ		0x008000	/** 32Kb */

/** Initializes the buffer structure.
 * @return: return 0 on success, -1 on malloc error
 */
int buffer_init(struct buffer_t *b) {
	const uint32_t len = 4096;

	b->alloc = 0;
	b->buf = malloc(len);
	if (!b->buf) {
		fprintf(stderr, "malloc error");
		return -1;
	}
	b->alloc = len;
	b->offset = 0;
	b->end = 0;

	return 0;
}

/** Frees any memory used for the buffer.*/
void buffer_free(struct buffer_t *b) {
	if (b->alloc > 0 && b->buf != NULL) {
		memset(b->buf, 0, b->alloc);
		b->alloc = 0;
		free(b->buf);
		b->buf = NULL;
	}
}

/**
 * Clears any data from the buffer, making it empty.  This does not actually
 * zero the memory.
 */
void buffer_clear(struct buffer_t *b) {
	b->offset = 0;
	b->end = 0;
}

/** Appends data to the buffer, expanding it if necessary. */
void buffer_append(struct buffer_t *b, const void *data, uint32_t len) {
	void *p;
	p = buffer_append_space(b, len);
	memcpy(p, data, len);
}

static int buffer_compact(struct buffer_t *b) {
	/**
	 * If the buffer is quite empty, but all data is at the end, move the
	 * data to the beginning.
	 */
	if (b->offset > MIN(b->alloc, BUFFER_MAX_CHUNK)) {
		memmove(b->buf, b->buf + b->offset,
				b->end - b->offset);
		b->end -= b->offset;
		b->offset = 0;
		return (1);
	}
	return (0);
}

/**
 * Appends space to the buffer, expanding the buffer if necessary. This does
 * not actually copy the data into the buffer, but instead returns a pointer
 * to the allocated region.
 */
void *buffer_append_space(struct buffer_t *b, uint32_t len) {
	uint32_t newlen;
	void *p;

	if (len > BUFFER_MAX_CHUNK)
		fprintf(stderr, "buffer_append_space: len %u not supported\n", len);

	/** If the buffer is empty, start using it from the beginning. */
	if (b->offset == b->end) {
		b->offset = 0;
		b->end = 0;
	}
restart:
	/** If there is enough space to store all data, store it now. */
	if (b->end + len < b->alloc) {
		p = b->buf + b->end;
		b->end += len;
		return p;
	}

	/** Compact data back to the start of the buffer if necessary */
	if (buffer_compact(b))
		goto restart;

	/** Increase the size of the buffer and retry. */
	newlen = roundup(b->alloc + len, BUFFER_ALLOCSZ);
	if (newlen > BUFFER_MAX_LEN)
		fprintf(stderr, "buffer_append_space: alloc %u not supported\n", newlen);
	if(b->buf)
		b->buf = realloc(b->buf, newlen);
	else
		b->buf = malloc(newlen);
	if(!b->buf) {
		fprintf(stderr, "realloc error");
	}
	b->alloc = newlen;
	goto restart;
	return NULL;
	/** NOTREACHED */
}

/**
 * Check whether an allocation of 'len' will fit in the buffer
 * This must follow the same math as buffer_append_space
 */
int buffer_check_alloc(struct buffer_t *b, uint32_t len) {
	if (b->offset == b->end) {
		b->offset = 0;
		b->end = 0;
	}
restart:
	if (b->end + len < b->alloc)
		return (1);
	if (buffer_compact(b))
		goto restart;
	if (roundup(b->alloc + len, BUFFER_ALLOCSZ) <= BUFFER_MAX_LEN)
		return (1);
	return (0);
}

/** Returns the number of bytes of data in the buffer. */
uint32_t buffer_len(const struct buffer_t *b) {
	return b->end - b->offset;
}

/** Returns the maximum number of bytes of data that may be in the buffer. */
uint32_t buffer_get_max_len(void) {
	return (BUFFER_MAX_LEN);
}

/** Gets data from the beginning of the buffer. */
int buffer_get_ret(struct buffer_t *b, void *buf, uint32_t len) {
	if (len > b->end - b->offset) {
		fprintf(stderr, "buffer_get_ret: trying to get more bytes %d than in buffer %d",
				len, b->end - b->offset);
		return (-1);
	}
	memcpy(buf, b->buf + b->offset, len);
	b->offset += len;
	return (0);
}

void buffer_get(struct buffer_t *b, void *buf, uint32_t len) {
	if (buffer_get_ret(b, buf, len) == -1)
		fprintf(stderr, "buffer_get: buffer error");
}

/** Consumes the given number of bytes from the beginning of the buffer. */
int buffer_consume_ret(struct buffer_t *b, uint32_t bytes) {
	if (bytes > b->end - b->offset) {
		fprintf(stderr, "buffer_consume_ret: trying to get more bytes than in buffer");
		return (-1);
	}
	b->offset += bytes;
	return (0);
}

void buffer_consume(struct buffer_t *b, uint32_t bytes) {
	if (buffer_consume_ret(b, bytes) == -1)
		fprintf(stderr, "buffer_consume: buffer error");
}

/** Consumes the given number of bytes from the end of the buffer. */
int buffer_consume_end_ret(struct buffer_t *b, uint32_t bytes) {
	if (bytes > b->end - b->offset)
		return (-1);
	b->end -= bytes;
	return (0);
}

void buffer_consume_end(struct buffer_t *b, uint32_t bytes) {
	if (buffer_consume_end_ret(b, bytes) == -1)
		fprintf(stderr, "buffer_consume_end: trying to get more bytes than in buffer");
}

/** Returns a pointer to the first used byte in the buffer. */
void *buffer_ptr(const struct buffer_t *b) {
	return b->buf + b->offset;
}

/** Dumps the contents of the buffer to stderr.*/
void buffer_dump(const struct buffer_t *b) {
	uint32_t i;
	uint8_t *ucp = b->buf;

	for (i = b->offset; i < b->end; i++) {
		fprintf(stderr, "%02x", ucp[i]);
		if ((i - b->offset) % 16 == 15)
			fprintf(stderr, "\r\n");
		else if ((i - b->offset) % 2 == 1)
			fprintf(stderr, " ");
	}
	fprintf(stderr, "\r\n");
}

