#pragma once

#include "../global/global.h"

#include "../peripheral/uart.h"
#include "../peripheral/flash.h"
#include "../peripheral/dac.h"
#include "../peripheral/adc.h"
#include <stdlib.h>

void auto_approach_begin (void);
void auto_approach_state_machine (void);
void auto_approach_count(void);
bool auto_approach_serial_read(void);
void auto_approach_get_info(void);
void auto_approach_measure(void);
void auto_approach_send_info(void);
void auto_approach_send_info_payload(void);

