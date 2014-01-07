/*
 * mpsse.c
 *
 *  Created on: 2012-10-26
 *      Author: dlrc
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "mpsse.h"
#include "ftdi_io.h"
#include "debug.h"

//#define FTIO_LIBFTDI1
#ifdef FTIO_LIBFTDI1
#include "ftdi_io_ftdi1.h"
#else
#include "ftdi_io_libftdi.h"
#endif

struct usb_info {
	const struct ftdi_io *ftio;
	unsigned short vid;
	unsigned short pid;
	char serial[32];
};

struct usb_info ftdev = {
#ifdef FTIO_LIBFTDI1
		.ftio = &ftdi_io_ftdi1,
#else
		.ftio = &ftdi_io_libftdi,
#endif
		.vid = 0x0403, //
		.pid = 0x6014, //
		.serial = "SNP00001", //
		};

/**
 * write, if error, exit
 */
int mpsse_write(const void *buf, int len) {
	int ret;
	ret = ftdev.ftio->write(buf, len);
	if (ret != len) {
		/* check if device is plugged out */
		if (errno == ENODEV) {
			fprintf(stderr, "device unpluged out\n");
			exit(errno);
		}
		perror("mpsse_write");
		return -1;
	}
	return len;
}

/**
 * read len bytes data to buf, if error, exit
 */
int mpsse_read(void *buf, int len) {
	int rlen = len;
	while (len > 0) {
		int ret;
		ret = ftdev.ftio->read(buf, len);
		if (ret < 0) {
			/* check if device is plugged out */
			if (errno == ENODEV) {
				fprintf(stderr, "device unpluged out\n");
				exit(errno);
			}
			perror("mpsse_read");
			return -1;
		}
		len -= ret;
	}
	return rlen;
}

static void mpsse_config(void) {
	int ret;
	int pos = 0;
	unsigned char buf[128];

	//disable divide by 5(use 60MHz master clock)
	buf[pos++] = 0x8A;

	//disable adaptive clocking
	buf[pos++] = 0x97;

	//disable three-phase clocking
	buf[pos++] = 0x8D;

	ret = ftdev.ftio->write(buf, pos);
	if (ret != pos) {
		perror("ftdev.ftio->write");
	}
	pos = 0;

	// Set initial states of the MPSSE interface - low byte, both pin directions and output values
	//    Pin name    Signal  Direction    Config  Initial State    Config
	//    ADBUS0    SCLK   	output    1    		       	0
	//    ADBUS1    DO   	output    1    		       	0
	//    ADBUS2    DI   	input     0            		0
	//    ADBUS3    CS   	output    1    		       	0
	//    ADBUS4    GPIOL0  output    1            		0
	//    ADBUS5    GPIOL1  output    1            		0
	//    ADBUS6    GPIOL2  output    1            		0
	//    ADBUS7    GPIOL3  output    1            		0
	buf[pos++] = 0x80;
	buf[pos++] = 0x00; // Initial state config above
	buf[pos++] = 0xfB; // Direction config above

	ret = ftdev.ftio->write(buf, pos);
	if (ret != pos) {
		perror("ftdev.ftio->write");
	}
	pos = 0;

	// Set initial states of the MPSSE interface - high byte, both pin directions and output values
	//    Pin name    Signal  Direction    Config  Initial State    Config
	//    ACBUS0    GPIOH0  output    1            0
	//    ACBUS1    GPIOH1  output    1            0
	//    ACBUS2    GPIOH2  output    1            0
	//    ACBUS3    GPIOH3  output    1            1 (RXLED)
	//    ACBUS4    GPIOH4  output    1            1 (TXLED)
	//    ACBUS5    GPIOH5  output    1            0
	//    ACBUS6    GPIOH6  output    1            0
	//    ACBUS7    GPIOH7  output    1            0

	// Set data bits low-byte of MPSSE port
	buf[pos++] = 0x82;
	buf[pos++] = 0x18; //0x18;// Initial state config above
	buf[pos++] = 0xff; // Direction config above

	ret = ftdev.ftio->write(buf, pos);
	if (ret != pos) {
		perror("ftdev.ftio->write");
	}
	pos = 0;

	/*
	 * Set TCK frequency
	 * 0x0000 30 MHz
	 * 0x0001 15 Mhz
	 * 0x0002 10 MHz (max speed for msp430 jtag)
	 * 0x0003 7.5 MHz
	 * 0x0004 6 MHz
	 * 0x0005 5.000 MHz
	 * 0x0006 4.285 MHz
	 * 0x0007 3.750 MHz
	 * 0x0008 3.333 MHz
	 * 0x0009 3.000 MHz max speed for msp430 flash
	 * NOTE: fet430uif is 2MHz
	 */
	// TCK = 60MHz /((1 + [(1 +0xValueH*256) OR 0xValueL])*2)
	// Value of clock divisor, SCL Frequency = 60/((1+clk_div)*2) (MHz)
	unsigned short clk_div = 0x0005; //max speed
	buf[pos++] = 0x86; //Command to set clock divisor
	buf[pos++] = clk_div & 0x00ff; //Set 0xValueL of clock divisor
	buf[pos++] = (clk_div >> 8) & 0x00ff;

	ret = ftdev.ftio->write(buf, pos);
	if (ret != pos) {
		perror("ftdev.ftio->write");
	}
	pos = 0;

#if 1
	// Disable internal loop-back
	buf[pos++] = 0x85;
#else
	//This will connect the TDI/DO output to the TDO/DI input for loopback testing
	buf[pos++] = 0x84;
#endif
	ret = ftdev.ftio->write(buf, pos);
	if (ret != pos) {
		perror("ftdev.ftio->write");
	}
	pos = 0;
}

/**
 * @return: 0 success, -1 error
 */
int mpsse_open(const char *usb_serial) {
	int ret;
	strncpy(ftdev.serial, usb_serial, 32);
	ret = ftdev.ftio->open(ftdev.vid, ftdev.pid, ftdev.serial);
	if (ret < 0) {
		Error("cant open ftdi\n");
		return -1;
	}

	ret = ftdev.ftio->serial_mode(115200);
	if (ret < 0) {
		Error("cant open serial\n");
		ftdev.ftio->close();
		return -1;
	}

	int success;
	for (success = 3; success > 0; success--) {
		ret = ftdev.ftio->mpsse_mode();
		if (ret == 0)
			break;
	}
	if (!success) {
		Error("setup mpsse error");
		return -1;
	}

	mpsse_config();
	return 0;
}

int mpsse_close(void) {
	ftdev.ftio->serial_mode(115200);
	ftdev.ftio->close();
	return 0;
}

