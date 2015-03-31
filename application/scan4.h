#pragma once

#include "../global/global.h"

#ifdef configMEMS_4ACT

#include "../peripheral/uart.h"
#include "../peripheral/flash.h"
#include "../peripheral/dac.h"
#include "../peripheral/adc.h"

#include "../system/pid.h"
#include "../system/pga_4ch_PGA4311.h"

void scan4_init (void);
void scan4_get_data (void);
void scan4_start (void);
void scan4_step (void);
bool scan4_get_dac_data (void);
void s4_set_sample_cnt (u8 smaple_cnt);
void s4_set_dwell_t_ms (u8 dwell_ms);

#endif
