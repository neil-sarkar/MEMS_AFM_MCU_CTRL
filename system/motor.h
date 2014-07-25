#pragma once

#include <stdlib.h>
#include "../global/global.h"
#include "../peripheral/dac.h"
#include "../peripheral/adc.h"
#include "../peripheral/uart.h"

#define BRK_CHAR 0x71 // 'q' character

typedef enum 
{
	mtr_fwd,
	mtr_bwd
} mtrdir;

void mtr_init (void);

u8 mtr_set_pw(u8 pw);

u8 mtr_set_dir (mtrdir dir);

__inline u8 mtr_step (void);
u8 mtr_auto_approach (us16 setpoint, us16 setpoint_error);

void mtr_handler (void);

