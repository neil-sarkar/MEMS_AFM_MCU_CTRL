#pragma once

#include "../global/global.h"
#include "../peripheral/uart.h"
#include "../peripheral/adc.h"
#include "../peripheral/dac.h"

void um_init (void);
void um_track (void);
void um_set_ki (float param);
void um_set_kp (float param);
