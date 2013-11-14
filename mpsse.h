/**
 * mpsse.h
 * copyright (C) 2013 lrs <ruishengleen@gmail.com>
 */
#ifndef MPSSE_H_
#define MPSSE_H_

/**
 * @return: 0 success, -1 error
 */
int mpsse_open(const char *usb_serial);
int mpsse_close(void);

/**
 * read len bytes data to buf, if error, exit
 */
void mpsse_read(void *buf, int len);

/**
 * write, if error, exit
 */
void mpsse_write(void *buf, int len);

#endif /* MPSSE_H_ */
