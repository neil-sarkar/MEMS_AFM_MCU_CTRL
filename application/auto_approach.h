#pragma once

#include "../global/global.h"

#include "../peripheral/uart.h"
#include "../peripheral/flash.h"
#include "../peripheral/dac.h"
#include "../peripheral/adc.h"

void auto_approach_begin (void);
void auto_approach_state_machine (void);
void auto_approach_count(void);
void auto_approach_get_info(void);
