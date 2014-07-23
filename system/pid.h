#ifndef __PID_H
#define __PID_H

#include "../global/global.h"

typedef enum
{
	AUTO,
	MANUAL,	
} pid_mode;

void compute (void);
void setSampleTime (u16 newSampleTime);
void setOutputLimits (float min, float max);
void setMode (pid_mode mode);
void initialize (void);
void pid_set_p (float param);
void pid_set_i (float param);
void pid_set_d (float param);
void pid_enable (bool enable);
void pid_set_setpoint (us16 new_setpoint);
void pid_handler(void);
void pid_wait_update (void);

#endif
