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

static int print_adc(struct adc_samples_result *rst, int num) {
	int i;
	char info;
	info = adc_get_con() == 0 ? 'L' : 'H';
	for (i = 0; i < num; i++) {
		printf("%c %f %f\n", info, rst[i].cur, rst[i].vol);
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
	char *serial = "SNAP0001";
	int times = 1000;
	struct adc_samples_result adc[ADC_NUMBERS];
	int sample_rate = 100;
	if (argc > 1)
		serial = argv[1];
	if (argc > 2)
		sample_rate = atoi(argv[2]);
	if (argc > 3)
		times = atoi(argv[3]);
	if (argc == 2 && strstr(argv[1], "-h") != NULL){
		fprintf(stdout, "usage:\n"
				"%s <usb serial> <sample rate> <read times>\n"
				"\texample: %s SNP00074 50 1000\n"
				"\texample: %s SNP00074 100 1000\n\n", argv[0],argv[0],argv[0]);
		//exit(1);
	}

	printf("serial...........: %s\n", serial);
	printf("sample rate......: %-d\n", sample_rate);
	printf("read.............: %-d\n", times);
	if (adc_open(serial, sample_rate) != 0) {
		fprintf(stderr, "failed to open device\n");
		exit(EXIT_FAILURE);
	}

	adc_set_con(1);
	times = 1000 * times * sample_rate / ADC_NUMBERS;
	while (times-- > 0) {
		if (sample_rate < 10) {
			for (i = 0; i < ADC_NUMBERS; i++)
				adc_read(adc+i, 1);
		} else {
			adc_read(adc, ADC_NUMBERS);
		}
		//filter_shake(adc, num);
		print_adc(adc, ADC_NUMBERS);
	}

	adc_close();
	return 0;
}
