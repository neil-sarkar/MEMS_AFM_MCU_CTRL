#ifndef __MAIN_H
#define __MAIN_H

#include "../global/global.h"

/*
TODO

 - Change u16 from unsigned int to unsigned short
 - Split up files properly
 - Change busy-wait functions to include _wait_ in definition
 - Understand interrupt priorities and ensure they're not a problem
 - use memcpy instead for handling floats
 - create macros for terminal characters
 - add config file to specify pin assignments
 - remove uart/adc/dac.h from pid?
 - Handle all input in a single TERMINAL file
 - put every single macro definition in the config file
 - add a gpio driver
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

#endif
