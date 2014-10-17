#ifndef __ADC_H
#define __ADC_H

#include "../global/global.h"

#define ADC_MAX_V		2.5f
#define ADC_MAX			4095

typedef enum
{
	adc0,
	adc1,
	adc2,
	adc3,
	adc4,
	adc5,
	adc6,
	adc7,
	adc8,
	adc9,
	padc0,
	padc1,
} adc;

typedef enum
{
	adc_single_ended,
	adc_diff,
	adc_pseudo_diff,
} adc_mode;

typedef enum
{
	adc_CONVSTn,
	adc_timer0,
	adc_timer1,
	adc_sw,
	adc_cont,
	adc_PLA,
} adc_conv;

void adc_init(void);

void adc_set_pga(adc channel, u8 gain);

u16 adc_get_avg_val (const u16 samples);

u16 adc_wait_get_avgw_val (adc channel, const u16 samples, const u16 wait);

u16 adc_wait_get_reading(adc channel);

#endif
