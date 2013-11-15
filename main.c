/**
 * main.c
 * copyright (C) 2013 lrs <ruishengleen@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "ltc1407a.h"

int main(int argc, char **argv) {
	char *serial = argv[1];
	int ilh_con = atoi(argv[2]);
	int times = atoi(argv[3]);
	int i;

	struct adc_samples_result adc[ADC_NUMBERS] = {0};

	if (argc != 4) {
		fprintf(stderr, "usage:\n"
				"%s <usb serial> <i-l-h-con> <read times>\n"
				"example: %s SNP00074 0 10\n"
				"example: %s SNP00074 1 10\n", argv[0],argv[0],argv[0]);
		exit(1);
	}

	if (adc_open(serial) != 0) {
		fprintf(stderr, "failed to open device\n");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < times; i++) {
		adc_read(adc);
	}

	adc_close();
	return 0;
}
