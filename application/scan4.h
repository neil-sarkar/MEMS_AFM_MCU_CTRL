#pragma once

#include "../global/global.h"
#include "../peripheral/pga_4ch.h"
#include "../peripheral/uart.h"
#include "../peripheral/flash.h"
#include "../peripheral/dac.h"

void scan4_init (void);
void scan4_get_data (void);
void scan4_start (void);
void scan4_step (void);
bool scan4_get_dac_data (void);
void scan4_send_dac_data (void);
