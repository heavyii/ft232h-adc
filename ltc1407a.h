/**
 * ltc1407a.h
 * copyright (C) 2013 lrs <ruishengleen@gmail.com>
 */
#ifndef LTC1407A_H_
#define LTC1407A_H_

#define ADC_NUMBERS  50

struct adc_samples_result {
	float cur;
	float vol;
};

struct adc_dst {
	uint16_t vol;
	uint16_t cur;
};

/**
 * rate: sample rate (khz)
 * @return: 0 on success, -1 on error
 */
int adc_open(const char *serial, int rate);
void adc_close(void);

/**
 * read adc data to rst[ADC_NUMBERS]
 */
int adc_read(struct adc_samples_result *rst, int num);

void adc_set_con(int value);
int adc_get_con(void);

#endif /* LTC1407A_H_ */
