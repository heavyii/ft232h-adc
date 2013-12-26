/**
 * main.c
 * copyright (C) 2013 lrs <ruishengleen@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>
#include "ltc1407a.h"

static int print_adc(struct adc_samples_result *rst, int num, float cur_cv) {
	int i;
	char info;
	info = adc_get_con() == 0 ? 'L' : 'H';
	for (i = 0; i < num; i++) {
		printf("%c %f %f\n", info, rst[i].cur * cur_cv, rst[i].vol);
	}
	return 0;
}

static int filter_shake(struct adc_samples_result *rst, int num) {
	int i;
	const float cur_shake = 0.00015;
	const float vol_shake = 0.00015;
	const float cur_maxcnt = 80;
	const float vol_maxcnt = 80;
	static float cur = 0;
	static float vol = 0;
	static int cur_cnt = 0;
	static int vol_cnt = 0;
	for (i = 0; i < num; i++) {
		float cur_delta = cur - rst[i].cur;
		float vol_delta = vol - rst[i].vol;
		if (cur_delta < cur_shake && cur_delta > -cur_shake && cur_cnt++ < cur_maxcnt) {
			rst[i].cur = cur;
		} else {
			cur = rst[i].cur;
			cur_cnt = 0;
		}

		if (vol_delta < vol_shake && vol_delta > -vol_shake && vol_cnt++ < vol_maxcnt) {
			rst[i].vol = vol;
		} else {
			vol = rst[i].vol;
			vol_cnt = 0;
		}
	}
	return 0;
}

int main(int argc, char **argv) {
	int i;
	char *serial = "ADC0002";
	int times = 1;
	float cur_cv = 0.0925;
	int ilh_con = 1;
	struct adc_samples_result adc[ADC_NUMBERS];
	int sample_rate = 100;
	if (argc > 1)
		serial = argv[1];
	if (argc > 2)
		ilh_con = atoi(argv[2]);
	if (argc > 3)
		sample_rate = atoi(argv[3]);
	if (argc > 4)
		times = atoi(argv[4]);
	if (argc > 5)
		cur_cv = atof(argv[5]);

	if (argc == 2 && strstr(argv[1], "-h") != NULL){
		fprintf(stdout, "usage:\n"
				"%s <usb serial> <i-l-h-con> <sample rate> <read times>\n"
				"\texample: %s SNP00074 0 1 10\n"
				"\texample: %s SNP00074 1 1 10\n\n", argv[0],argv[0],argv[0]);
		//exit(1);
	}

	printf("serial...........: %s\n", serial);
	printf("ilh_con..........: %-d\n", ilh_con);
	printf("sample rate......: %-d\n", sample_rate);
	printf("read.............: %-d\n", times);
	printf("current conv.....: %f\n", cur_cv);
	if (adc_open(serial, sample_rate) != 0) {
		fprintf(stderr, "failed to open device\n");
		exit(EXIT_FAILURE);
	}

	adc_set_con(ilh_con);
	//adc_read(adc, num);
	for (i = 0; i < times; i++) {
		if (sample_rate < 10) {
			for (i = 0; i < ADC_NUMBERS; i++)
				adc_read(adc+i, 1);
		} else {
			adc_read(adc, ADC_NUMBERS);
		}
		//filter_shake(adc, num);
		print_adc(adc, ADC_NUMBERS, cur_cv);
	}

	adc_close();
	return 0;
}
