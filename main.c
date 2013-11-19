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

int main(int argc, char **argv) {
	int i;
	char *serial = "ADC0002";
	int times = 1;
	int ilh_con = 1;
	struct adc_samples_result adc[ADC_NUMBERS];

	if (argc > 1)
		serial = argv[1];
	if (argc > 2)
		ilh_con = atoi(argv[2]);
	if (argc > 3)
		times = atoi(argv[3]);

	if (argc == 2 && strstr(argv[1], "-h") != NULL){
		fprintf(stdout, "usage:\n"
				"%s <usb serial> <i-l-h-con> <read times>\n"
				"\texample: %s SNP00074 0 10\n"
				"\texample: %s SNP00074 1 10\n\n", argv[0],argv[0],argv[0]);
		//exit(1);
	}

	printf("serial...........: %s\n", serial);
	printf("ilh_con..........: %-d\n", ilh_con);
	printf("read.............: %-d\n", times);
	if (adc_open(serial) != 0) {
		fprintf(stderr, "failed to open device\n");
		exit(EXIT_FAILURE);
	}

	adc_set_con(ilh_con);
	for (i = 0; i < times; i++) {
		adc_read(adc);
	}

	adc_close();
	return 0;
}
