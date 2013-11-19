/*
 * ftdi_io.c
 *
 *  Created on: May 30, 2013
 *      Author: heavey
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ftdi.h>
#include "debug.h"
#include "ftdi_io_libftdi.h"

static struct ftdi_context *ftdi = NULL;

static int ftdi_write(const void *buf, int len) {
	return ftdi_write_data(ftdi, (unsigned char *)buf, len);
}

static int ftdi_read(void *buf, int len) {
	return ftdi_read_data(ftdi, (unsigned char *)buf, len);
}

static void ftdi_fatal(struct ftdi_context *ftdi, char *str) {
	Error("%s: %s\n", str, ftdi_get_error_string(ftdi));
	ftdi_free(ftdi);
}

static int ftdi_dtr(int state) {
	int ret;
	if (state)
		state = 0;
	else
		state = 1;
	ret = ftdi_setdtr(ftdi, state);
	if (ret == 0) {
		//Debug("DTR %d\n", status);
	} else if (ret == -1)
		Error("set dtr failed\n");
	else if (ret == -2)
		Error("usb dev unavailable\n");

	return ret;
}


static int ftdi_rts(int state) {
	int ret;
	if (state)
		state = 0;
	else
		state = 1;

	ret = ftdi_setrts(ftdi, state);
	if (ret == 0) {
		//Error("RTS %d\n", status);
	} else if (ret == -1)
		Error("set dtr failed\n");
	else if (ret == -2)
		Error("usb dev unavailable\n");

	return ret;
}

static int ftdi_open(int vendor, int product,
		const char* serial) {
	if ((ftdi = ftdi_new()) == 0) {
		Error("ftdi_bew failed\n");
		return -1;
	}

	ftdi_set_interface(ftdi, INTERFACE_ANY);

	if (ftdi_usb_open_desc(ftdi, vendor, product, NULL, serial) < 0) {
		ftdi_fatal(ftdi, "Can't open ftdi device");
		ftdi = NULL;
		return -1;
	}

	return 0;
}

/**
 * ftdi_serial - open ftdi chip and set serial 8N1 no flow control and baudrate
 * @return: < 0 error, 0 all fine
 */
static int ftdi_serial(int baudrate) {
	int ret;
	ftdi_usb_reset(ftdi);
	ftdi_usb_purge_buffers(ftdi);
	Debug("reset and purge buffer\n");

	//turn back to normal mode
	if (ftdi->bitbang_mode != 0) {
		ftdi_disable_bitbang(ftdi);
		usleep(50);
	}
	ftdi_usb_reset(ftdi);
	ftdi_usb_purge_buffers(ftdi);

	ret = ftdi_set_line_property(ftdi, 8, STOP_BIT_1, NONE);
	if (ret < 0) {
		ftdi_fatal(ftdi, "Can't set line parameters");
		ftdi = NULL;
		return -1;
	}

	ftdi_setflowctrl(ftdi, SIO_DISABLE_FLOW_CTRL);
	ret = ftdi_set_baudrate(ftdi, baudrate);
	if (ret == -1) {
		perror("invalid baudrate");
	} else if (ret == -2) {
		perror("setting baudrate failed");
	} else if (ret == -3) {
		perror("USB device unavailable");
	}
	return 0;
}

static int ftdi_flush(void) {
	return ftdi_usb_purge_buffers(ftdi);
}

static int ftdi_mpsse(void) {
	int ret = 0;
	Debug("configuring for MPSSE...\n");
	//reset usb device
	ftdi_usb_reset(ftdi);

	//clear buffer
	ftdi_usb_purge_buffers(ftdi);

	//TODO: set usb request transfer sizes to 64k
	ret = ftdi_read_data_set_chunksize(ftdi, 65536);
	ret |= ftdi_write_data_set_chunksize(ftdi, 65536);
	if(ret != 0){
		Error("ftdi data set chunksize error\n");
	}
	//disable event and error characters
	ret = ftdi_set_event_char(ftdi, 0, 0);
	ret |= ftdi_set_error_char(ftdi, 0, 0);

	//sets the read and write timeouts in milliseconds
	ftdi->usb_read_timeout = 100;
	ftdi->usb_write_timeout = 100;

	//set the latency time
	ret |= ftdi_set_latency_timer(ftdi, 1);

	//reset controller
	ret |= ftdi_set_bitmode(ftdi, 0x00, BITMODE_RESET);

	//enable mpsse mode
	ret |= ftdi_set_bitmode(ftdi, 0x00, BITMODE_MPSSE);

	if (ret != 0) {
		Error("error in initializing the MPSSE\n");
		return -1;
	}

	//wait for all usb stuff to complete and work
	usleep(5000);

	int i;
	for (i = 0; i < 3; i++) {
		//send a bad command to test mpsse
		unsigned char buf[2];
		buf[0] = 0xAA;

		ret = ftdi_write_data(ftdi, buf, 1);
		if (ret != 1) {
			perror("ftdi_write_data");
		}

		int j;
		for (j = 0; j < 3; j++) {
			ret = ftdi_read_data(ftdi, buf, 2);
			if (ret == 2)
				break;
		}

		ftdi_usb_purge_buffers(ftdi);

		Debug("read %d bytes: 0x%x 0x%x\n", ret, buf[0], buf[1]);
		if (buf[0] == 0xFA && buf[1] == 0xAA)
			return 0;
	}

	Error("error in checking the MPSSE\n");
	return -1;
}

static void ftdi_close(void){
	if (ftdi == NULL)
		return;
	//reset controller
	ftdi_set_bitmode(ftdi, 0x00, BITMODE_BITBANG);

	//turn back to normal mode
	if (ftdi->bitbang_mode != 0) {
		ftdi_disable_bitbang(ftdi);
		usleep(50);
		ftdi_setflowctrl(ftdi, SIO_DISABLE_FLOW_CTRL);
	}

	ftdi_usb_reset(ftdi);
	ftdi_usb_purge_buffers(ftdi);

	ftdi_usb_reset(ftdi);
	ftdi_usb_purge_buffers(ftdi);

	ftdi_usb_close(ftdi);
	//close ftdi
	ftdi_free(ftdi);
	ftdi = NULL;
}

const struct ftdi_io ftdi_io_libftdi = {
		.open = ftdi_open,
		.close = ftdi_close,
		.serial_mode = ftdi_serial,
		.mpsse_mode = ftdi_mpsse,
		.read = ftdi_read,
		.write = ftdi_write,
		.flush = ftdi_flush,
		.setdtr = ftdi_dtr,
		.setrts = ftdi_rts,
};
