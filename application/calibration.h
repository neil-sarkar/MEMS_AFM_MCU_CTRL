#pragma once

#include <math.h>
#include "../peripheral/adc.h"
#include "../peripheral/dac.h"
#include "../peripheral/uart.h"
#include "../global/global.h"

#define ADC_SAMPLES 32

#define NUM_COEFF 3

typedef struct {
	dac out_dac;
	adc in_adc;
	adc z_adc;
	us16 max_voltage;
	float pv_rel [3];
	float vp_rel [3];
	float rv_indirect_rel [3];
	float vr_indirect_rel [3];
} Actuator;

// Makes this inline to speed up calculations
float pwr (Actuator* act, float volt);
float volt (Actuator* act, float pwr);

void init_act (Actuator* act, dac out_dac, adc in_adc, adc z_adc);
void calibrate_actuator (Actuator* actuator, us16 max_voltage);
void calibrate_z_actuator (Actuator* actuator, us16 max_voltage);


