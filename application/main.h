#ifndef __MAIN_H
#define __MAIN_H

#include "../global/global.h"

/*
ISSUES

 - iss01: 	change u16 from unsigned int to unsigned short
 - iss02: 	split up files properly
 - iss03: 	change busy-wait functions to include _wait_ in definition
 - iss04: 	understand interrupt priorities and ensure they're not a problem
 - iss05: 	use memcpy instead for handling floats
 - iss06: 	create macros for terminal characters
 - iss07: 	add config file to specify pin assignments
 - iss08: 	remove uart/adc/dac.h from pid?
 - iss09: 	handle all input in a single TERMINAL file
 - iss10: 	put every single macro definition in the config file
 - iss11: 	add a gpio driver
*/

/* 
Functional TODOs

 - todo01: 	update calibration parameters while doing a line scan.
 - todo02: 	add step-size control for the course approach.
 - todo03: 	manual course approach: dither the z-actuator from 0-1V (offset) as to indicate 
 			when tip is in contact while approaching with the AFM manually (while looking at the force signal).
 - todo04: 	add calibration for z-actuator. Ramp the z-offset while measuring its resistance, with the z-amp set to 0.
 - todo05: 	do a line scan while measuring the z-actuator resistance. Apply 1V to the z-offzet (z-amp=0) and perform an 
 			isothermal linescan. The z-actuator resistance should remain flat, but if it doesn't then manual adjustment 
			can be made to the calibration parameters until it is flat. 
 - todo06: 	add a controllable delay for a) in between points in the frequency scan and b) calibration.
 - todo07: 	DDS control: once an operating frequency has been selected set the DDS range to +/- 250Hz from the selected 
 			frequency, then maximize the frequency resolution in that range. The frequency can then be adjusted by known 
			increments.
 - todo08:	add comments to all function prototypes
 - todo09:	tage a number to each version of the code and an accessor method for the client to retrieve the version #
*/

/* Nice to haves and enhancements
 - enh01:	cast all macro definitions.
*/

/***** function definitions *****/

void print_menu(void);

void set_dac_max (void);
void write_dac(void);
void read_dac(void);

void read_adc(void);
void read_z (void);

void set_actuators(void);

void set_p_gain (void);
void set_i_gain (void);
void set_d_gain (void);
void set_pid_setpoint (void);

void freq_sweep (void);
void freq_sweep_dds(void);

void set_dir(char dirchar);
void set_pw (void);
void single_pulse(void);
void cont_pulse(void);
void auto_approach (void);

void device_calibration (void);

void set_scan_wait (void);
void configure_scan (void);
void start_scan (void);
void step_scan (void);

void set_pga (void);

void set_pv_rel_manual_a (void);
void set_pv_rel_manual_b (void);
void set_pv_rel_manual_c (void);

void calib_get_freq_amp (void);
void calib_freq_amp(u16* amp_max, u32* freq);

#endif
