#ifndef __CONFIG_H
#define __CONFIG_H

#define DAC_BFRD1			dac0
#define DAC_BFRD2			dac1
#define DAC_BR2				dac2
#define DAC_ZAMP			dac3
#define DAC_BR1				dac4
#define DAC_BFRD3			dac5
#define DAC_ZOFFSET_FINE	dac6
#define DAC_Y1				dac7
#define DAC_ZOFFSET_COARSE	dac8
#define DAC_Y2				dac9
#define DAC_X1				dac10
#define DAC_X2				dac11
#define DAC_ZVCO			DAC_BFRD1//unused by default on board

#define ADC_PHASE			adc0
#define ADC_ZOFFSET			adc2
#define ADC_X1				adc3
#define ADC_SPARE1			adc4
#define ADC_ZAMP			adc5

#define ADC_Y1				adc6
#define ADC_X2				adc7
#define ADC_Y2				adc8

#define ADC_MIRROR			padc0

#endif

#define PID_INPUT		ADC_ZAMP
#define PID_OUTPUT		DAC_ZOFFSET_FINE

