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

//#define DEBUG_RAW

#define ADCMASK 0x3fff

#if 0
/* led off */
static const uint8_t ILH_CON_LOW[] =  { 0x82, 0x18, 0xff };
static const uint8_t ILH_CON_HIGH[] = { 0x82, 0x38, 0xff };
#else
/* led on */
static const uint8_t ILH_CON_LOW[] =  { 0x82, 0x00, 0xff };
static const uint8_t ILH_CON_HIGH[] = { 0x82, 0x10, 0xff };
#endif

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
static uint8_t ADC_SAMPLE[] = {
	/* f=5MHz, T=200ns
	 * CLK_sample = 34, T_sample = 34*200 ns = 6800ns = 6.8us, f_sample = 147KHz
	 *
	 * clk = 5000 / f - 34; //f is sample rate (KHz)
	 */
	/* clk */
	/* 0x8e, 0x03, */
	/* S0: SCK=0 CS=1 */
	0x80, 0x08, 0xfB,

	/* S1: SCK=1 CS=0 */
	0x80, 0x01, 0xfB,

	/* read 4 bytes */
	0x20, 0x03, 0x00,

	/* delay, Clock For n x 8 bits
	 * x8F, LengthL, LengthH,
	 */
	0x8f, 0x00, 0x00};

struct adc_samples_raw {
	uint16_t cur;
	uint16_t vol;
};

struct adc_info {
	int init;
	int ilh_con;
	uint8_t cmds[ADC_NUMBERS][sizeof(ADC_SAMPLE)];
	struct adc_samples_raw raw[ADC_NUMBERS]; 	/* raw data */
	int sample_rate;
	int filter_winn;
	float cur_trans;
};

struct adc_info global_adc = { 0 };

static inline double adc_conv(x) {
	/* result = 3*x/2^14 */
	return (2.5 * ((x) & ADCMASK) / ADCMASK);
}

void adc_set_con(int value) {
	global_adc.ilh_con = value;
	if (value == 0) {
		global_adc.cur_trans = 0.036;
		mpsse_write(ILH_CON_LOW, sizeof(ILH_CON_LOW));
	} else {
		global_adc.cur_trans = 0.0473;
		mpsse_write(ILH_CON_HIGH, sizeof(ILH_CON_HIGH));
	}
}

int adc_get_con(void) {
	return global_adc.ilh_con;
}

/**
 * rate: sample rate (khz)
 */
int adc_open(const char *serial, int rate) {
	int i;
	int clk = 0;
	global_adc.sample_rate = rate;
	global_adc.filter_winn = 0;
	if (rate > 50)
		global_adc.filter_winn = rate / 4;

	if (mpsse_open(serial) != 0) {
		fprintf(stderr, "failed to open device\n");
		return -1;
	}

	clk = (5000/rate - 34)/8;
	if (clk < 0)
		clk = 0;
	ADC_SAMPLE[10] = clk;
	ADC_SAMPLE[11] = clk >> 8;

	for (i = 0; i < ADC_NUMBERS; i++) {
		memcpy(global_adc.cmds[i], ADC_SAMPLE, sizeof(ADC_SAMPLE));
	}

	adc_set_con(1);
	return 0;
}

void adc_close(void) {
	mpsse_close();
}

#ifdef DEBUG_RAW
static void debug_print_adc(const struct adc_samples_result *rst, int num) {
	int i;
	char info;
	info = adc_get_con() == 0 ? 'L' : 'H';
	for (i = 0; i < num; i++) {
		printf("%c RAW(%u, %0.4f, %u) ", info, global_adc.raw[i].cur, adc_conv(global_adc.raw[i].cur), global_adc.raw[i].vol);
		printf("RST(%f, %f)\n", rst[i].cur, rst[i].vol);
	}
}
#endif

static int filter_window_ave(struct adc_samples_result *rst, int num, int WINN) {
	int i;
	static int filed = 0;
	static struct adc_samples_result windows[100];
	static struct adc_samples_result sum = { 0.0, 0.0 };
	static int pos = 0;
	static int winn = 0;

	//refill for different WINN
	if (winn != WINN) {
		winn = WINN;
		filed = 0;
	}

	if (!filed) {
		int i;
		for(i = 0; pos < WINN && i < num; i++) {
			windows[pos].cur = rst[i].cur;
			windows[pos].vol = rst[i].vol;
			pos++;
		}

		if (pos < WINN)
			return 0;

		for(pos = 0; pos < WINN; pos++) {
			sum.cur += windows[pos].cur;
			sum.vol += windows[pos].vol;
		}
		filed = 1;
		pos = 0;
	}

	for (i = 0; i < num; i++) {
		sum.cur -= windows[pos].cur;
		sum.cur += rst[i].cur;
		windows[pos].cur = rst[i].cur;
		rst[i].cur = sum.cur / WINN;

		sum.vol -= windows[pos].vol;
		sum.vol += rst[i].vol;
		windows[pos].vol = rst[i].vol;
		rst[i].vol = sum.vol / WINN;

		pos++;
		if (pos >= WINN)
			pos = 0;
	}

	return 0;
}

int adc_read(struct adc_samples_result *rst, int num) {
	static int i = 0;
	if (num > ADC_NUMBERS) {
		fprintf(stderr, "num too big, limit num <= %d\n", ADC_NUMBERS);
		return -1;
	}

	if (mpsse_write(global_adc.cmds, sizeof(global_adc.cmds[0]) * num) < 0)
		return -1;
	if (mpsse_read(global_adc.raw, sizeof(global_adc.raw[0]) * num) < 0)
		return -1;

	for (i = 0; i < num; i++) {
		/* ADC raw data is big-endian */
		global_adc.raw[i].cur = ntohs(global_adc.raw[i].cur);
		global_adc.raw[i].vol = ntohs(global_adc.raw[i].vol);

		rst[i].cur = adc_conv(global_adc.raw[i].cur) * global_adc.cur_trans;
		rst[i].vol = adc_conv(global_adc.raw[i].vol) * 4;
	}
	/* XXX: drop the second sample */
	if (num >= 3) {
		rst[1].vol = (rst[0].vol + rst[2].vol) / 2;
		rst[1].cur = (rst[0].cur + rst[2].cur) / 2;
	}

	if (rst[0].cur - 0.012 > 0.0001) {
		if (adc_get_con() == 0)
			adc_set_con(1);
	} else {
		if (adc_get_con() != 0)
			adc_set_con(0);
	}

	if (global_adc.filter_winn > 1)
		filter_window_ave(rst, num, global_adc.filter_winn);
	return num;
}
