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
#include "math.h"
/**
 * print hex stream
 */
void printhex(const void *buf, int len) {
	int i = 0;
	const char *p = (const char *) buf;
	for (i = 0; i < len; i++) {
		printf(" %02hhx", *p++);
	}
	putchar('\n');
}

static const unsigned char ADC_CMD_HIGH[] = {
	/* L-H-HIGH */
	0x82, 0x18, 0xff,

	/* SCK=0 CS=1 */
	0x80, 0x08, 0xfB,

	/* SCK=1 CS=0 */
	0x80, 0x01, 0xfB,

	/* read 4 bytes */
	0x20, 0x03,	0x00,

	/* SCK=1 CS=0 */
	0x80, 0x01,	0xfB
};

static const unsigned char ADC_CMD_LOW[] = {
	/* L-H-HIGH */
	0x82, 0x08, 0xff,

	/* SCK=0 CS=1 */
	0x80, 0x08, 0xfB,

	/* SCK=1 CS=0 */
	0x80, 0x01, 0xfB,

	/* read 4 bytes */
	0x20, 0x03,	0x00,

	/* SCK=1 CS=0 */
	0x80, 0x01,	0xfB
};


static inline double convert_adc(int x) {
	return 3*x/pow(2.0, 14.0);
}

static void adc_read_high(void) {
	#define AMOUNT  10
	int i;
	unsigned char buf[AMOUNT][sizeof(ADC_CMD_HIGH)];
	unsigned short result[AMOUNT*2];

	for (i = 0; i < AMOUNT; i++) {
		memcpy(buf[i], ADC_CMD_HIGH, sizeof(ADC_CMD_HIGH));
	}
	mpsse_write(buf, sizeof(buf));

	mpsse_read(&result, sizeof(result));

	/*
	 *
	 * 总的 Iload = Iout + Isense = Vout/90.91 + Vout/10/Rsense = Vout * 1.011
	 * 当  I-L-H-CON = 0时，Rsense = 100;当  I-L-H-CON = 1时，Rsense = 0.1;
	 */
//	for (i = 0; i < AMOUNT; i++) {
//		double current = convert_adc(cur);
//		double voltage = convert_adc(vol);
//		double rsense = 0.1;
//		printf("H RAW(%d, %d)\n", cur, vol);
//		printf("H RST(%f, %f)\n\n", current/90.91 + 1.0*current/10/rsense, voltage);
//	}
}

static void adc_read_low(void) {
	unsigned char buf[1024];
	int pos = 0;

	//set LHCON high
	buf[pos++] = 0x82;
	buf[pos++] = 0x08; //0x18;// Initial state config above
	buf[pos++] = 0xff; // Direction config above

	//SCK=0 CS=1
	buf[pos++] = 0x80;
	buf[pos++] = 0x08; // Initial state config above
	buf[pos++] = 0xfB; // Direction config above

	//SCK=1 CS=0
	buf[pos++] = 0x80;
	buf[pos++] = 0x01; // Initial state config above
	buf[pos++] = 0xfB; // Direction config above

	//read 4 bytes
	buf[pos++] = 0x20;
	buf[pos++] = 0x03;
	buf[pos++] = 0x00;

	//SCK=1 CS=0
	buf[pos++] = 0x80;
	buf[pos++] = 0x01; // Initial state config above
	buf[pos++] = 0xfB; // Direction config above

	mpsse_write(buf, pos);
	pos = 0;

	mpsse_read(buf, 4);
	buf[0] &= 0x3f;
	buf[2] &= 0x3f;

	/*
	 *
	 * 总的 Iload = Iout + Isense = Vout/90.91 + Vout/10/Rsense = Vout * 1.011
	 * 当  I-L-H-CON = 0时，Rsense = 100;当  I-L-H-CON = 1时，Rsense = 0.1;
	 */
	//printhex(buf, 4);
	//vol/0x3f x 3 x 4
	int vol = ((short)buf[2] << 8) | buf[3];
	int cur = ((short)buf[0] << 8) | buf[1];

	double current = convert_adc(cur);
	double voltage = convert_adc(vol);
	double rsense = 100;
	printf("L RAW(%d, %d)\n", cur, vol);
	printf("L RST(%f, %f)\n\n", current/90.91 + 1.0*current/10/rsense, voltage);
}

int main(int argc, char **argv)
{
	int i;
	if (mpsse_open(argv[1]) != 0) {
		fprintf(stderr, "failed to open device\n");
		exit(EXIT_FAILURE);
	}
	for (i = 0; i < 1000; i++)  {
		adc_read_low();
		adc_read_high();
	}
	puts("OK");

	mpsse_close();
	return 0;
}
