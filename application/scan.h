#pragma once

#include <math.h>
#include "calibration.h"
#include "../global/global.h"
#include "../peripheral/flash.h"
#include "../peripheral/dac.h"
#include "../peripheral/adc.h"
#include "../peripheral/uart.h"
#include "../system/pid.h"

/* Device calibration constants */
//#define ADC_GAIN 300.3939f
//#define CURNT_SENSE_REST 1.00f
#define SCAN_OUT_SIZE 8 // 6 bytes per point

#define MEASURE_BIT 0x1000
#define BFR_SIZE PAGE_SIZE/2 

typedef struct sample_data sample_data;

void init_scanner (Actuator* left_act, Actuator* right_act, Actuator* z_act);
u8 scan_configure (const u16 vmin_line,
			const u16 vmin_scan,
			const u16 vmax,
			const u16 numpts,
			const u16 numlines);
void scan_start (void);
void scan_step (void);

void z_init_sample (void);
void z_set_samples (u16 num_samples);
u16 z_sample (void);
void z_write_data (void);
