#pragma once

#include "../global/global.h"
#include "../peripheral/dac.h"
#include "../peripheral/adc.h"

#define MOTOR_DIR BIT22
#define MOTOR_PWR BIT23

#define MOTOR_DIR_DD BIT30
#define MOTOR_PWR_DD BIT31

typedef enum 
{
	mtr_fwd,
	mtr_bwd
} mtrdir;

void mtr_init (void);

u8 mtr_set_pw(u8 pw);

u8 mtr_set_dir (mtrdir dir);

u8 mtr_step (void);
u8 mtr_auto_approach (void);

void mtr_handler (void);
