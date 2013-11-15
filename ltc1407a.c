/**
 * ltc1407a.c
 * copyright (C) 2013 lrs <ruishengleen@gmail.com>
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
#include "ltc1407a.h"


#define ADCMASK 0x3fff

static const uint8_t ILH_CON_LOW[] =  { 0x82, 0x18, 0xff };
static const uint8_t ILH_CON_HIGH[] = { 0x82, 0x38, 0xff };

/*
 * MPSSE: f=30MHz T=33ns
 *
 * SCK CS
 * |   |_
 * |_   _|  0x80, 0x08, 0xfB,
 *  _| |    0x80, 0x01, 0xfB,
 * |   |
 * 32bits   0x20, 0x03, 0x00
 */
static const uint8_t ADC_SAMPLE[] = {
	/* S0: SCK=0 CS=1 */
	0x80, 0x08, 0xfB,

	/* S1: SCK=1 CS=0 */
	0x80, 0x01, 0xfB,

	/* read 4 bytes */
	0x20, 0x03, 0x00};

struct adc_samples_raw {
	uint16_t cur;
	uint16_t vol;
};

struct adc_info {
	int init;
	int ilh_con;
	uint8_t cmds[ADC_NUMBERS][sizeof(ADC_SAMPLE)];
	struct adc_samples_raw raw[ADC_NUMBERS]; 	/* raw data */
};

struct adc_info global_adc = { 0 };

/**
 * print hex stream
 */
void dumphex(const char *msg, const void *buf, int len) {
	int i = 0;
	const char *p = (const char *) buf;
	if (msg)
		puts(msg);
	for (i = 0; i < len; i++) {
		printf(" %02hhx", *p++);
	}
	putchar('\n');
}

static inline double adc_conv(x) {
	/* result = 3*x/2^14 */
	return (3.3 * ((x) & ADCMASK) / ADCMASK);
}

void adc_set_con(int value) {
	global_adc.ilh_con = value;
	if (value == 0)
		mpsse_write(ILH_CON_LOW, sizeof(ILH_CON_LOW));
	else
		mpsse_write(ILH_CON_HIGH, sizeof(ILH_CON_HIGH));
}

int adc_get_con(void) {
	return global_adc.ilh_con;
}

int adc_open(const char *serial) {
	int i;
	if (mpsse_open(serial) != 0) {
		fprintf(stderr, "failed to open device\n");
		return -1;
	}

	for (i = 0; i < ADC_NUMBERS; i++) {
		memcpy(global_adc.cmds[i], ADC_SAMPLE, sizeof(ADC_SAMPLE));
	}
	adc_set_con(0);
	return 0;
}

void adc_close(void) {
	mpsse_close();
}

static void debug_print_adc(const struct adc_samples_result rst[ADC_NUMBERS]) {
	int i;
	char info;
	info = adc_get_con() == 0 ? 'L' : 'H';
	for (i = 0; i < ADC_NUMBERS; i++) {
		printf("%c RAW(%u, %u)\n", info, global_adc.raw[i].cur, global_adc.raw[i].vol);
		printf("%c RST(%f, %f)\n\n", info, rst[i].cur, rst[i].vol);
	}
}

void adc_read(struct adc_samples_result rst[ADC_NUMBERS]) {
	static int i = 0;
	float rsense;

	//dumphex("CMD", global_adc.cmds, sizeof(global_adc.cmds));
	mpsse_write(global_adc.cmds, sizeof(global_adc.cmds));
	mpsse_read(global_adc.raw, sizeof(global_adc.raw));

	/*
	 * 总的 Iload = Iout + Isense = Vout/90.91 + Vout/10/Rsense = Vout * 1.011
	 * 当  I-L-H-CON = 0时，Rsense = 100;
	 * 当  I-L-H-CON = 1时，Rsense = 0.1;
	 */
	rsense = global_adc.ilh_con == 0 ? 100.0 : 0.1;
	for (i = 0; i < ADC_NUMBERS; i++) {
		float cur;
		/* ADC raw data is big-endian */
		global_adc.raw[i].cur = ntohs(global_adc.raw[i].cur) & ADCMASK;
		global_adc.raw[i].vol = ntohs(global_adc.raw[i].vol) & ADCMASK;

		cur = adc_conv(global_adc.raw[i].cur);
		rst[i].cur = cur / 90.91 + cur / 10 / rsense;
		rst[i].vol = adc_conv(global_adc.raw[i].vol);
	}

#if 1
	debug_print_adc(rst);
#endif
}

