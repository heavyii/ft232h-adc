#ifndef BUFFER_H
#define BUFFER_H

#include <stdint.h>

struct buffer_t {
	uint8_t *buf; /** struct buffer_t for data. */
	uint32_t alloc; /** Number of bytes allocated for data. */
	uint32_t offset; /** Offset of first byte containing data. */
	uint32_t end; /** Offset of last byte containing data. */
};

int buffer_init(struct buffer_t *b);
void buffer_clear(struct buffer_t *b);
void buffer_free(struct buffer_t *b);

uint32_t buffer_len(const struct buffer_t *b);
void *buffer_ptr(const struct buffer_t *b);

void buffer_append(struct buffer_t *b, const void *data, uint32_t len);
void *buffer_append_space(struct buffer_t *b, uint32_t len);

int buffer_check_alloc(struct buffer_t *b, uint32_t len);

void buffer_get(struct buffer_t *b, void *buf, uint32_t len);

void buffer_consume(struct buffer_t *b, uint32_t len);
void buffer_consume_end(struct buffer_t *b, uint32_t len);

void buffer_dump(const struct buffer_t *b);

int buffer_get_ret(struct buffer_t *b, void *buf, uint32_t len);
int buffer_consume_ret(struct buffer_t *b, uint32_t len);
int buffer_consume_end_ret(struct buffer_t *b, uint32_t len);

uint32_t buffer_get_max_len(void);

#endif				/** BUFFER_H */

