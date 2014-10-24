#pragma once

#include <math.h>
#include "calibration.h"
#include "../global/global.h"
#include "../peripheral/flash.h"
#include "../peripheral/dac.h"
#include "../peripheral/adc.h"
#include "../peripheral/uart.h"

/* Device calibration constants */
#define MEASURE_BIT 0x1000
#define BFR_SIZE PAGE_SIZE/2 

typedef struct sample_data sample_data;

void init_scanner (Actuator* left_act, Actuator* right_act);
u8 scan_configure (const u16 vmin_line,
			const u16 vmin_scan,
			const u16 vmax,
			const u16 numpts,
			const u16 numlines);
void scan_start (void);
void scan_step (void);

void scan_handler (void);
