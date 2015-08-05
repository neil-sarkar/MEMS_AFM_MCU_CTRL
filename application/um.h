#pragma once

#include "../global/global.h"
#include "../peripheral/uart.h"
#include "../peripheral/adc.h"
#include "../peripheral/dac.h"

void um_init (void);
void um_track (void);
void um_raster (void);
void um_raster_step(void);
