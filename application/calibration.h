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
	u16 max_voltage;
	float pv_rel [3];
	float vp_rel [3];
	float rv_indirect_rel [3];
	float vr_indirect_rel [3];
} Actuator;

/* State information about scanning. Used to restart scan
	from last point of scanning */
// TODO: make this better?
typedef struct {
	u16 vmin;
	u16 vmax;
	u16 numpts;
	u32 baseline_points;
	u32 i;
	u32 j;
	u32 k;
	u32 adr;
	dac left_act;
	dac right_act;
	u8 	freq_hz;
	u32	freq_ld;
	u32 step_ld;
} scan_params;

// Makes this inline to speed up calculations
float pwr (Actuator* act, u16 volt);
float volt (Actuator* act, float pwr);

void init_act (Actuator* act, dac out_dac, adc in_adc);
void calibrate_actuator (Actuator* actuator, u16 max_voltage);
void calibrate_z_actuator (Actuator* actuator, u16 max_voltage);

void set_pv_rel_a (Actuator* act, float a);
void set_pv_rel_b (Actuator* act, float b);
void set_pv_rel_c (Actuator* act, float c);
