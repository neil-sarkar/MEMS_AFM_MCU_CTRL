#ifndef __CONFIG_H
#define __CONFIG_H

/* Identify which board is used
 * to use Mahdi's board's pinout, comment BOARD_d and un-comment BOARD_m
 * to use Duncan's board's pinout, comment BOARD_m and un-comment BOARD_d
 * Note that ONLY one of these directives must be un-commented at a time
*/
//#define BOARD_m
#define BOARD_m_assem
//#define BOARD_v2

//#define ZAMP_PADC

#ifdef BOARD_m
#define DAC_X1			dac10	
#define DAC_X2			dac6//switch back to DAC9. Switched for fine Z control
#define DAC_Y1			dac8
#define DAC_Y2			dac7
#define DAC_BR1			dac3
#define DAC_BR2			dac2
#define DAC_ZVCO		dac4
#define DAC_ZOFFSET		dac9//switch back to DAC6. Switched for fine Z control
#define DAC_ZAMP		dac5

#define ADC_X1			adc4
#define ADC_X2			adc2
#define ADC_Y1			adc1
#define ADC_Y2			adc0
#define ADC_ZOFFSET		adc5
#define ADC_PHASE		adc6

#ifdef ZAMP_PADC
#define ADC_ZAMP		padc0
#else
#define ADC_ZAMP		adc3
#endif

#endif

/* Green board with onboard mcu */
#ifdef BOARD_m_assem
#define DAC_X1				dac11	
#define DAC_X2				dac7//Swapped with DAC10 for fine Z control
#define DAC_Y1				dac9
#define DAC_Y2				dac8
#define DAC_BR1				dac4
#define DAC_BR2				dac3
#define DAC_ZVCO			dac5
#define DAC_ZOFFSET_FINE	dac10//swapped with DAC7 for fine Z control
#define DAC_ZOFFSET_COARSE	dac11
#define DAC_ZAMP			dac6

#define ADC_X1				adc4
#define ADC_X2				adc2
#define ADC_Y1				adc1
#define ADC_Y2				adc0
#define ADC_ZOFFSET			adc5
#define ADC_PHASE			adc6

#ifdef ZAMP_PADC
#define ADC_ZAMP		padc0
#else
#define ADC_ZAMP		adc3
#endif

#endif

#ifdef BOARD_v2
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
#ifdef ZAMP_PADC
#error "Still not set PADC"
#define ADC_ZAMP		padc0
#else
#define ADC_ZAMP		adc5
#endif
#define ADC_Y1				adc6
#define ADC_X2				adc7
#define ADC_Y2				adc8

#endif

#define PID_INPUT		ADC_ZAMP
#define PID_OUTPUT		DAC_ZOFFSET_FINE

#endif
