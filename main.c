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
#include <stdint.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "mpsse.h"
#include "math.h"

static volatile int read_thread_started = 0;
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

#define ADCMASK 0x3fff


#define AMOUNT  1

static const unsigned char ADC_CMD_HIGH[] = {
	/* L-H-HIGH */
	0x82, 0x38, 0xff,

	/* SCK=0 CS=1 */
	0x80, 0x08, 0xfB,

	/* SCK=1 CS=0 */
	0x80, 0x01, 0xfB,

	/* read 4 bytes */
	0x20, 0x03, 0x00,

	/* SCK=1 CS=0 */
	0x80, 0x01, 0xfB };

static const unsigned char ADC_CMD_LOW[] = {
	/* L-H-LOW */
	0x82, 0x18, 0xff,

	/* SCK=0 CS=1 */
	0x80, 0x08, 0xfB,

	/* SCK=1 CS=0 */
	0x80, 0x01, 0xfB,

	/* read 4 bytes */
	0x20, 0x03, 0x00,

	/* SCK=1 CS=0 */
	0x80, 0x01, 0xfB };

static inline double adc_conv(x) {
	return (3.0 * (ntohs(x) & ADCMASK) / ADCMASK);
}

static void adc_ILH_con(int value) {
	static const unsigned char ILH_CON_LOW[] =  { 0x82, 0x18, 0xff };
	static const unsigned char ILH_CON_HIGH[] = { 0x82, 0x38, 0xff };
	if (value)
		mpsse_write(ILH_CON_HIGH, sizeof(ILH_CON_HIGH));
	else
		mpsse_write(ILH_CON_HIGH, sizeof(ILH_CON_HIGH));
}

static void *adc_read_thread(void *args) {
	int ret = 0;
	unsigned char dst[10240];
	int count = 0;
	read_thread_started = 1;
	while (1) {
		ret = mpsse_read(dst, sizeof(dst));
		count += ret;
		printf("read %d\r", count);
		fflush(stdout);
	}
}

static void *adc_write_thread(void *args) {
	int i;
	unsigned char buf[AMOUNT][sizeof(ADC_CMD_HIGH)];
	for (i = 0; i < AMOUNT; i++) {
		memcpy(buf[i], ADC_CMD_HIGH, sizeof(ADC_CMD_HIGH));
	}

//	while (!read_thread_started) {
//		usleep(1000);
//	}
	int count = 0;
	unsigned char dst[10240];
	while (1) {
		int ret;
		mpsse_write(*buf, sizeof(buf));
		ret = mpsse_read(dst, sizeof(dst));
		count += ret;
		if (count > 1024*1024*10)
			break;
		printf("read %d\r", count);
		fflush(stdout);
	}
}

static void adc_read_high(void) {
	int i;
	unsigned char buf[AMOUNT][sizeof(ADC_CMD_HIGH)];
	unsigned short result[AMOUNT * 2];

	for (i = 0; i < AMOUNT; i++) {
		memcpy(buf[i], ADC_CMD_HIGH, sizeof(ADC_CMD_HIGH));
	}

	mpsse_write(*buf, sizeof(buf));

	static int count = 0;
	count += mpsse_read(result, sizeof(result));
	printf("\r%d", count/4);
	fflush(stdout);
	return;
	/*
	 *
	 * 总的 Iload = Iout + Isense = Vout/90.91 + Vout/10/Rsense = Vout * 1.011
	 * 当  I-L-H-CON = 0时，Rsense = 100;当  I-L-H-CON = 1时，Rsense = 0.1;
	 */
	for (i = 0; i < AMOUNT; i++) {
		unsigned short cur = ntohs(result[2 * i]) & ADCMASK;
		unsigned short vol = ntohs(result[2 * i + 1]) & ADCMASK;

		double current = adc_conv(cur);
		double voltage = adc_conv(vol);

		double rsense = 0.1;
		printf("H RAW(%u, %u)\n", cur, vol);
		printf("H RST(%f, %f)\n\n",
				current / 90.91 + 1.0 * current / 10 / rsense, voltage);
	}
}

static void adc_read_low(void) {
	int i;
	unsigned char buf[AMOUNT][sizeof(ADC_CMD_LOW)];
	unsigned short result[AMOUNT * 2];

	for (i = 0; i < AMOUNT; i++) {
		memcpy(buf[i], ADC_CMD_LOW, sizeof(ADC_CMD_LOW));
	}
	mpsse_write(*buf, sizeof(buf));
	mpsse_read(result, sizeof(result));

	/*
	 *
	 * 总的 Iload = Iout + Isense = Vout/90.91 + Vout/10/Rsense = Vout * 1.011
	 * 当  I-L-H-CON = 0时，Rsense = 100;
	 * 当  I-L-H-CON = 1时，Rsense = 0.1;
	 */
	for (i = 0; i < AMOUNT; i++) {
		unsigned short cur = ntohs(result[2 * i]) & ADCMASK;
		unsigned short vol = ntohs(result[2 * i + 1]) & ADCMASK;

		double current = adc_conv(result[2 * i]);
		double voltage = adc_conv(result[2 * i + 1]);

		double rsense = 100;
		printf("L RAW(%u, %u)\n", cur, vol);
		printf("L RST(%f, %f)\n\n",
				current / 90.91 + 1.0 * current / 10 / rsense, voltage);
	}
}

static mpsse_clkout_test(void) {

}

int main(int argc, char **argv) {
	int i;
	if (mpsse_open(argv[1]) != 0) {
		fprintf(stderr, "failed to open device\n");
		exit(EXIT_FAILURE);
	}
#if 1
	int count = 0;
	for (i = 0; i < 10000; i++) {
		printf("\r%d", count);
		fflush(stdout);
		//adc_read_low();
		//adc_read_high();
		adc_ILH_con(1);
		count += 3;
	}

#else
	pthread_t threads[2];
	//pthread_create(&threads[0], NULL, adc_read_thread, NULL);
	pthread_create(&threads[1], NULL, adc_write_thread, NULL);
	pthread_join(threads[1], NULL);
	//pthread_join(threads[0], NULL);
#endif

	mpsse_close();
	return 0;
}
