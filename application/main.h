#ifndef __MAIN_H
#define __MAIN_H

#include "../global/global.h"

/***** function definitions *****/

void print_menu(void);

void set_dac_max (void);
void write_dac(void);
void read_dac(void);

void read_adc(void);
void set_actuators(void);

void freq_sweep_dds(void);

void device_calibration (void);

void set_scan_wait (void);
void configure_scan (void);

void set_pv_rel_manual_a (void);
void set_pv_rel_manual_b (void);
void set_pv_rel_manual_c (void);

void act_res_test (void);
void reset_mcu (void);

void set_pga (void);

void um_track (void);
void um_sweep(void);
void um_set_delta (void);
void um_init(void);
void um_track2 (void);

#endif
