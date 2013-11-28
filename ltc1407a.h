/**
 * ltc1407a.h
 * copyright (C) 2013 lrs <ruishengleen@gmail.com>
 */
#ifndef LTC1407A_H_
#define LTC1407A_H_

#define ADC_NUMBERS  80

struct adc_samples_result {
	float cur;
	float vol;
};

/**
 * @return: 0 success, -1 error
 */
int adc_open(const char *serial);
void adc_close(void);

/**
 * read adc data to rst[ADC_NUMBERS]
 */
void adc_read(struct adc_samples_result rst[ADC_NUMBERS]);

void adc_set_con(int value);
int adc_get_con(void);

#endif /* LTC1407A_H_ */
