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
void scan_configure (const u16 numpts);
void scan_start (void);
void scan_step (void);

void scan_handler (void);

void scan_set_freq (u8 frequency);
void scan_reset_state (void);
