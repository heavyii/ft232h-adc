/*
 * ftdi_io.h
 *
 *  Created on: May 30, 2013
 *      Author: heavey
 */

#ifndef FTDI_IO_H_
#define FTDI_IO_H_

typedef int (*ftdi_io_open_t)(int vendor, int product,
		const char* serial);
typedef void (*ftdi_io_close_t)(void);
typedef int (*ftdi_io_serial_mode_t)(int baudrate);
typedef int (*ftdi_io_mpsse_mode_t)(void);
typedef int (*ftdi_io_read_t)(void *buf, int len);
typedef int (*ftdi_io_write_t)(void *buf, int len);
typedef int (*ftdi_io_flush_t)(void);
typedef int (*ftdi_io_setbaudrate_t)(int baudrate);
typedef int (*ftdi_io_set_dtr_t)(int state);
typedef int (*ftdi_io_set_rts_t)(int state);

struct ftdi_io {
	ftdi_io_open_t open;
	ftdi_io_close_t close;

	ftdi_io_serial_mode_t serial_mode;
	ftdi_io_mpsse_mode_t mpsse_mode;

	ftdi_io_read_t read;
	ftdi_io_write_t write;
	ftdi_io_flush_t flush;

	ftdi_io_setbaudrate_t setbaud;
	ftdi_io_set_dtr_t setdtr;
	ftdi_io_set_rts_t setrts;
};

#endif /* FTDI_IO_H_ */
