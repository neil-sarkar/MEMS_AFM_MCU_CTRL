/* TODO:
* used configuration between motors
*/

#include "main.h"

#include "../peripheral/uart.h"
#include "../peripheral/adc.h"
#include "../peripheral/dac.h"
#include "../peripheral/flash.h"
#include "../peripheral/flash.h"
#include "../peripheral/wire3.h"

#include "../system/dds_AD5932.h"
#include "../system/pid.h"
#include "../system/motor.h"
#include "../system/stpr_DRV8834.h"

#include "../system/pga_1ch_LM1971.h"
#include "../system/pga_4ch_PGA4311.h"
#include "../system/dds_AD9837.h"
#include "../system/pga_8ch_CS3308.h"

#include "calibration.h"
#include "scan2.h"
#include "scan4.h"
#include "scan4_ortho.h"

#ifdef configSYS_DDS_AD9837
tyVctHndlr    DDS     	= (tyVctHndlr)dds_98_handler;
#endif

#ifdef configSYS_DDS_AD5932
tyVctHndlr    DDS     	= (tyVctHndlr)dds_handler;
#endif

tyVctHndlr    PID     	= (tyVctHndlr)pid_handler;
tyVctHndlr 	  UART			= (tyVctHndlr)uart_handler;

// System level handlers
#ifdef configMOTOR_PCB
tyVctHndlr	  MTR				= (tyVctHndlr)mtr_handler;
#endif

#ifdef configMOTOR_STEPPER_DRV8834
tyVctHndlr	  MTR				= (tyVctHndlr)stpr_handler;
#endif

tyVctHndlr	  WIRE3			= (tyVctHndlr)wire3_handler;

#ifdef configSYS_DDS_AD5932						
extern int dds_inc_cnt;
#endif
#ifdef configSYS_DDS_AD9837
extern int dds_AD9837_inc_cnt;
#endif

#ifdef configMEMS_2ACT
static Actuator left_act;
static Actuator right_act;
static Actuator z_act;
#endif

static void sys_init (void);

#define SWEEP_MAX 4096

// DAC attenuators CS bits
#define DAC0_CS_DIR_BIT BIT29
#define DAC0_CS_BIT	BIT21

// Macro function to clear terminal screen
#define CLEAR() uart_write("\033c")

// Delay variable for the calibration routine
extern u8 calib_delay;

int main(void)
{
	u8 rx_char;
	u8 byte_l, byte_h;
	u16 byte_full;
	
	/*
	 * MCU Initialization				  
	 */

	/* Configure CPU Clock for 41.78MHz, CD=0 */
	POWKEY1 = 0x01;
	POWCON  = 0x00;
	POWKEY2 = 0xF4;

	/* initialize all relevant modules */
   	sys_init ();

	/*
	 * Main program loop
	 */
	while (true)
	{
		/* Enable to catch errors in UART
		if (uart_get_status () != UART_OK)
		{
			while (true);
		} */

		rx_char = uart_wait_get_char();
		
		switch (rx_char)
		{
			// Set DAC
			case 'a':
				write_dac();
				break;
			// Read DAC
			case 'b':
				read_dac();
				break;
			// Read ADC
			case 'c':
				read_adc();
				break;
			// Set Actuator Voltages
			case 'f':
				set_actuators();
				break;
			case 'g':
				pid_enable(true);
				break;
			case 'h':
				pid_enable(false);
				break;
			case 'p':
				set_p_gain();
				break;				
			case 'i':
				set_i_gain();
				break;				
			case 'd':
				set_d_gain();
				break;
			case 's':
				set_pid_setpoint();
				break;
#ifdef configMOTOR_PCB
			case 'j':
				set_pw();
				break;
			case 'k':
				set_dir('k');
				break;
			case 'l':
				set_dir('l');
				break;			
			case 'm':
				single_pulse();
				break;
			case 'v': 
				auto_approach();
				break;
#endif // configMOTOR_PCB
			
#ifdef configSYS_DDS_AD9837
			case 'n':
				dds_AD9837_get_data();
				break;			
			case 'q':
				freq_sweep_AD9837();
				break;
#endif
#ifdef configSYS_DDS_AD5932
			case 'r':
				freq_sweep_dds();
				break;
			case 't':
				dds_increment();
				break;
			case 'u':
				dds_get_data();
				break;
#endif

			case 'z':
				adc_set_pga(padc0, uart_wait_get_char());
				break;
			case 'e':
				read_z();
				break;
#ifdef configMEMS_2ACT
			case '!':
				set_scan_wait ();
				break;		 
			case '@':
				configure_scan ();
				break;
			case '#':
				start_scan ();
				break;
			case '^':
				step_scan ();
				break;
			case 'O':
				calib_delay = uart_wait_get_char ();
				break;
			case 'o':
				device_calibration ();
				break;
			case 'Z':
				set_send_back_cnt (uart_wait_get_char());
				break;
			case '(':
				s2_set_lvl_dir (uart_wait_get_char ());
				break;
#endif
			case '&':
				set_dac_max ();
				break;
#ifdef configSYS_PGA_LM1971_PGA4311
			case '*':
				set_pga ();
				break;
#endif
			case 'J':
				act_res_test ();
				break;
			case 'K':
				if (set_fine_speed(uart_wait_get_char()) == true)
					uart_set_char('o');
				else
					uart_set_char('f');
				break;
			case 'L':
				if (set_coarse_speed(uart_wait_get_char()) == true)
					uart_set_char('o');
				else
					uart_set_char('f');
				break;
			case 'N':
				force_curve ();
				break;
// 4scan spcific functions
#ifdef configMEMS_4ACT 
			case 'P':
				scan4_get_data ();
				break;
			case 'Q':
				scan4_start ();
				break;
			case 'R':
				scan4_step ();
				break;
			case 'S':
				if (scan4_get_dac_data () == true)
					uart_set_char('o');
				else
					uart_set_char('f');
				break;
			case 'w':
				s4_set_dwell_t_ms (uart_wait_get_char());
				break;
			case 'y':
				s4_set_sample_cnt (uart_wait_get_char());
				break;
			case 'Z':
				s4_set_send_back_cnt (uart_wait_get_char());
				break;
			case '(':
				s4_set_lvl_dir (uart_wait_get_char());
				break;
#endif

#ifdef configMEMS_4ACT_ORTHO
			case 'P':
				scan4_ortho_get_data ();
				break;
			case 'Q':
				scan4_ortho_start ();
				break;
			case 'R':
				scan4_ortho_step ();
				break;
			case 'S':
				if (scan4_ortho_get_dac_data () == true)
					uart_set_char('o');
				else
					uart_set_char('f');
				break;
			case 'w':
				s4_set_dwell_t_ms (uart_wait_get_char());
				break;
			case 'y':
				s4_set_sample_cnt (uart_wait_get_char());
				break;
			case 'Z':
				s4_set_send_back_cnt (uart_wait_get_char());
				break;
			case '(':
				s4_set_lvl_dir (uart_wait_get_char());
				break;			
#endif

#ifdef configSYS_PGA_CS3308 
			case 'T':
				pga_get_data();
				break;
			case 'U':
				mute(uart_wait_get_char());
				break;
#endif
			// TODO take these out?
			case 'A':
				//set_pv_rel_manual_a ();
				break;
			case 'B':
				//set_pv_rel_manual_b ();
				break;
			case 'C':
				//set_pv_rel_manual_c ();
				break;
			// Stepper motor
			case '0':
				stpr_set_step((STEP_MODE)uart_wait_get_char());
				break;
			case '1':
				stpr_step();	
				break;
#ifdef configMOTOR_STEPPER_DRV8834
			case '2':
				byte_l = uart_wait_get_char();
				byte_h = uart_wait_get_char();
				byte_full = (byte_h << 8) | byte_l;	
				stpr_set_speed(byte_full);	
				break;
			case '3':
				stpr_cont();	
				break;
			case '4':
				stpr_sleep();
				break;
			case '5':
				stpr_wake();
				break;			
			case '6':
				stpr_set_dir(uart_wait_get_char());
				break;
			case '7':
				stpr_exit_cont();
				break;
#endif // configMOTOR_STEPPER_DRV8834
		}
	}
}

/**********************************
		Initialization 
**********************************/

static void sys_init ()
{

	// TODO: choose from:
	// 2 pgas
	// 2 scanners
	// 2 dds'

	/* Initialize flash memory */
	flash_Init ();

   	/* Initialize SPI/DDS */
#ifdef configSYS_DDS_AD5932
	dds_spi_init();
#endif
#ifdef configSYS_DDS_AD9837
	dds_AD9837_spi_init();
#endif

	/* Initialize UART */
	uart_init();  

	/* Initialize ADC */
	adc_init();

	/* Configure all DACs */
	dac_set_range(dac0, dac_AVdd_AGND);
	dac_set_range(dac1, dac_AVdd_AGND);
	dac_set_range(dac2, dac_AVdd_AGND);
	dac_set_range(dac3, dac_AVdd_AGND);
	dac_set_range(dac4, dac_AVdd_AGND);
	dac_set_range(dac5, dac_AVdd_AGND);
	dac_set_range(dac6, dac_AVdd_AGND);
	dac_set_range(dac7, dac_AVdd_AGND);
	dac_set_range(dac8, dac_AVdd_AGND);
	dac_set_range(dac9, dac_AVdd_AGND);
	dac_set_range(dac10, dac_AVdd_AGND);
	dac_set_range(dac11, dac_AVdd_AGND);

	dac_init (dac0, dac_enable);
	dac_init (dac1, dac_enable);
	dac_init (dac2, dac_enable);
	dac_init (dac3, dac_enable);
	dac_init (dac4, dac_enable);
	dac_init (dac5, dac_enable);
	dac_init (dac6, dac_enable);
	dac_init (dac7, dac_enable);
	dac_init (dac8, dac_enable);
	dac_init (dac9, dac_enable);
	dac_init (dac10, dac_enable);
	dac_init (dac11, dac_enable);

	/* Init motor control */
#ifdef configMOTOR_PCB
	mtr_init ();
#endif // configMOTOR_PCB

#ifdef configMOTOR_STEPPER_DRV8834
	stpr_init ();
#endif // configMOTOR_STEPPER_DRV8834

#ifdef configSYS_PGA_LM1971_PGA4311
	/* Init DAC attenuators */
	pga_1ch_init (pga_fine);
	pga_1ch_init (pga_dds);
	pga_4ch_init ();		   
	
	pga_1ch_set (pga_fine, 127);
	pga_1ch_set (pga_dds, 127);
	pga_4ch_set (pga_x1, 192);
	pga_4ch_set (pga_x2, 192);
	pga_4ch_set (pga_y1, 192);
	pga_4ch_set (pga_y2, 192);
#endif // configSYS_PGA_LM1971_PGA4311

#ifdef configSYS_PGA_CS3308
	/* Init the 8-PGA */
	pga_init();
#endif

#ifdef configMEMS_2ACT	
	/* Init actuators */
	init_act (&left_act, DAC_Y1, ADC_Y1, ADC_ZOFFSET);
	init_act (&right_act, DAC_X1, ADC_X1, ADC_ZOFFSET);
	init_act (&z_act, DAC_ZOFFSET_FINE, ADC_ZOFFSET, ADC_Y1);

	init_scanner (&left_act, &right_act, &z_act);
#endif

#ifdef configMEMS_4ACT
 	scan4_init();
#endif

#ifdef configMEMS_4ACT_ORTHO
	scan4_ortho_init();
#endif

	/* Disable filter and PID */
	pid_enable(false);
}

#define FC_INITIAL_Z	2500
#define FC_STEP			5
#define FC_AVG_CNT		16
void force_curve (void)
{
	u16 dac_val;
	u16 adc_val;
	u32 delay = 10000;

   	dac_set_val(DAC_ZOFFSET_FINE, FC_INITIAL_Z);
	while (delay--) {}

	for (dac_val = FC_INITIAL_Z; dac_val > 0; dac_val -= FC_STEP)
	{
		dac_set_val(DAC_ZOFFSET_FINE, dac_val);

	   	adc_start_conv(ADC_ZAMP);
		adc_val = adc_get_avgw_val(FC_AVG_CNT, 300);

		uart_set_char(adc_val);
		uart_set_char((adc_val & 0x0F00) >> 8);

	   	adc_start_conv(ADC_PHASE);
		adc_val = adc_get_avgw_val(FC_AVG_CNT, 300);

		uart_set_char(adc_val);
		uart_set_char((adc_val & 0x0F00) >> 8);
	}

	for (dac_val = 0; dac_val < FC_INITIAL_Z; dac_val += FC_STEP)
	{
		dac_set_val(DAC_ZOFFSET_FINE, dac_val);
	   	
		adc_start_conv(ADC_ZAMP);
		adc_val = adc_get_avgw_val(FC_AVG_CNT, 300);

		uart_set_char(adc_val);
		uart_set_char((adc_val & 0x0F00) >> 8);	
		
	   	adc_start_conv(ADC_PHASE);
		adc_val = adc_get_avgw_val(FC_AVG_CNT, 300);

		uart_set_char(adc_val);
		uart_set_char((adc_val & 0x0F00) >> 8);	
	}		
}

extern u8 isPidOn;
extern u16 pid_input;
extern u16 pid_phase;
void read_z (void)
{
	u16 z_amp, z_offset, z_phase;

	if (isPidOn == 1)
	{
		z_amp 		= pid_input;
		z_offset 	= dac_get_val(DAC_ZOFFSET_FINE);
		z_phase 	= pid_phase;
	}
	else
	{
		adc_start_conv (ADC_ZAMP);
		z_amp 		= adc_get_val (); 
		
		z_offset	= dac_get_val(DAC_ZOFFSET_FINE);
		adc_start_conv(ADC_PHASE);
		z_phase 	= adc_get_val();
	}

	uart_set_char((u8)((z_amp) & 0xFF));
	uart_set_char((u8)((z_amp >> 8) & 0xFF));

	uart_set_char((u8)(z_offset & 0xFF));
	uart_set_char((u8)((z_offset >> 8) & 0xFF));

	uart_set_char((u8)(z_phase & 0xFF));
	uart_set_char((u8)((z_phase >> 8) & 0xFF));
}

#define MV_TO_ABS_200	248
#define FUDGE_FACTOR_X1	1047/0.038

void act_res_test (void)
{
	u16 x1, x2, y1, y2, z_c, z_amp;
	u16 adc_val;

	// save the actuator dac values
	x1 = dac_get_val(DAC_X1);
	x2 = dac_get_val(DAC_X2);
	y1 = dac_get_val(DAC_Y1);
	y2 = dac_get_val(DAC_Y2);
	z_c = dac_get_val(DAC_ZOFFSET_COARSE);
	z_amp = dac_get_val(DAC_ZAMP);

	// set all actuator dac values to 200mv
	dac_set_val(DAC_ZAMP, 0);	
	dac_set_val(DAC_X1, MV_TO_ABS_200);
	dac_set_val(DAC_X2, MV_TO_ABS_200);
	dac_set_val(DAC_Y1, MV_TO_ABS_200);
	dac_set_val(DAC_Y2, MV_TO_ABS_200);
	dac_set_val(DAC_ZOFFSET_COARSE, 62);

	// retrieve actuators' resistance values	
	adc_start_conv(ADC_X1);
//	adc_val = adc_get_val();
	adc_val = adc_get_avgw_val(32, 500);
		
	uart_set_char(adc_val & 0xFF);
	uart_set_char((adc_val & 0x0F00) >> 8);

	adc_start_conv(ADC_X2);
//	adc_val = adc_get_val();
	adc_val = adc_get_avgw_val(32, 500);
	
	uart_set_char(adc_val & 0xFF);
	uart_set_char((adc_val & 0x0F00) >> 8);

	adc_start_conv(ADC_Y1);
//	adc_val = adc_get_val();
	adc_val = adc_get_avgw_val(32, 500);
	
	uart_set_char(adc_val & 0xFF);
	uart_set_char((adc_val & 0x0F00) >> 8);

	adc_start_conv(ADC_Y2);
//	adc_val = adc_get_val();
	adc_val = adc_get_avgw_val(32, 500);
	
	uart_set_char(adc_val & 0xFF);
	uart_set_char((adc_val & 0x0F00) >> 8);

	adc_start_conv(ADC_ZOFFSET);
//	adc_val = adc_get_val();
	adc_val = adc_get_avgw_val(32, 500);
	
	uart_set_char(adc_val & 0xFF);
	uart_set_char((adc_val & 0x0F00) >> 8);

	// TODO: CONVERT FROM VOLTAGE TO RESISTANCE 

	// set actuator values to what they were originally	
	dac_set_val(DAC_X1, x1);
	dac_set_val(DAC_X2, x2);
	dac_set_val(DAC_Y1, y1);
	dac_set_val(DAC_Y2, y2);
	dac_set_val(DAC_ZAMP, z_amp);
	dac_set_val(DAC_ZOFFSET_COARSE, z_c);							
}
/*
void set_pv_rel_manual_a (void) 
{
	float coeff_a;
	u8 actuator;
	
	// determine which actuator is to be calibrated
   	actuator = uart_wait_get_char();

	// get calibration coefficients
	*((u8*)(&coeff_a) + 0) = uart_wait_get_char();
	*((u8*)(&coeff_a) + 1) = uart_wait_get_char();
	*((u8*)(&coeff_a) + 2) = uart_wait_get_char();	
	*((u8*)(&coeff_a) + 3) = uart_wait_get_char();

	switch (actuator)
	{
		case ('l'):
			set_pv_rel_a (&left_act, coeff_a); 
			break;
		case ('r'):
			set_pv_rel_a (&right_act, coeff_a);
			break;
		case ('z'):
			set_pv_rel_a (&z_act, coeff_a);
			break;
	}	
}

void set_pv_rel_manual_b (void) 
{
	float coeff_b;
	u8 actuator;
	
	// determine which actuator is to be calibrated
   	actuator = uart_wait_get_char();

	// get calibration coefficients
	*((u8*)(&coeff_b) + 0) = uart_wait_get_char();
	*((u8*)(&coeff_b) + 1) = uart_wait_get_char();
	*((u8*)(&coeff_b) + 2) = uart_wait_get_char();	
	*((u8*)(&coeff_b) + 3) = uart_wait_get_char();

	switch (actuator)
	{
		case ('l'):
			set_pv_rel_b (&left_act, coeff_b); 
			break;
		case ('r'):
			set_pv_rel_b (&right_act, coeff_b);
			break;
		case ('z'):
			set_pv_rel_b (&z_act, coeff_b);
			break;
	}	
}

void set_pv_rel_manual_c (void) 
{
	float coeff_c;
	u8 actuator;
	
	// determine which actuator is to be calibrated
   	actuator = uart_wait_get_char();

	// get calibration coefficient
	*((u8*)(&coeff_c) + 0) = uart_wait_get_char();
	*((u8*)(&coeff_c) + 1) = uart_wait_get_char();
	*((u8*)(&coeff_c) + 2) = uart_wait_get_char();	
	*((u8*)(&coeff_c) + 3) = uart_wait_get_char();

	switch (actuator)
	{
		case ('l'):
			set_pv_rel_c (&left_act, coeff_c); 
			break;
		case ('r'):
			set_pv_rel_c (&right_act, coeff_c);
			break;
		case ('z'):
			set_pv_rel_c (&z_act, coeff_c);
			break;
	}	
}
*/
void set_dac_max (void)
{
	dac dac_ch;
	u16 new_dac_limit;

	// Get DAC channel
	dac_ch = (dac)uart_wait_get_char();
	// Get new DAC limit
	uart_wait_get_bytes((u8*)(&new_dac_limit), 2);

	if (dac_set_limit (dac_ch, new_dac_limit) == 0){
		uart_write ("o");
	} else {
		uart_write ("f");
	}
}

void write_dac(void) 
{
	dac dac_ch;
	u8 dac_val_l;
	u8 dac_val_h;
	u16 new_dac_val;

	/* Get DAC channel */
	dac_ch = (dac)uart_wait_get_char();
	/* Get new DAC value */
	dac_val_l = uart_wait_get_char();
	dac_val_h = uart_wait_get_char();
	
	new_dac_val = ((dac_val_h << 8) | dac_val_l) & 0x0FFF;

	//write new value
	dac_set_val (dac_ch, new_dac_val);
}

void read_adc(void)
{
	adc adc_ch;
	u16 adc_val;

	// Get ADC channel
	adc_ch = (adc)uart_wait_get_char();

	// Read ADC
	adc_start_conv(adc_ch);
	adc_val = adc_get_val();
	
	uart_set_char(adc_val & 0xFF);
	uart_set_char((adc_val >> 8) & 0xFF);
}

void read_dac(void)
{
	dac dac_ch;
	u16 dac_val;

	// Get DAC channel
	dac_ch = (dac)uart_wait_get_char();
	
	// Get DAC value
	dac_val = dac_get_val(dac_ch);

	uart_set_char(dac_val & 0xFF);
	uart_set_char((dac_val >> 8) & 0xFF);		
}

void set_actuators(void)
{
	u8 dac1_l, dac1_h, dac2_l, dac2_h;		  

	dac1_l = uart_wait_get_char();
	dac1_h = uart_wait_get_char();
	dac2_l = uart_wait_get_char();
	dac2_h = uart_wait_get_char();

	// Write to DACs corresponding to left/right actuators
	// TODO Confirm dac channels
	dac_set_val (DAC_X1, ((dac1_h << 8) | dac1_l) & 0x0FFF);
	dac_set_val (DAC_Y1, ((dac2_h << 8) | dac2_l) & 0x0FFF);
}
		
void set_p_gain (void)
{
	// Receive paramter value as a single-precision floating point number (32-bit)
	float param;

	*((u8*)(&param) + 0) = uart_wait_get_char();
	*((u8*)(&param) + 1) = uart_wait_get_char();
	*((u8*)(&param) + 2) = uart_wait_get_char();	
	*((u8*)(&param) + 3) = uart_wait_get_char();

	pid_set_p (param);
}

void set_i_gain (void)
{
	// Receive paramter value as a single-precision floating point number (32-bit)
	float param;

	*((u8*)(&param) + 0) = uart_wait_get_char();
	*((u8*)(&param) + 1) = uart_wait_get_char();
	*((u8*)(&param) + 2) = uart_wait_get_char();	
	*((u8*)(&param) + 3) = uart_wait_get_char();

	pid_set_i (param);
}

void set_d_gain (void)
{
	float param;

	*((u8*)(&param) + 0) = uart_wait_get_char();
	*((u8*)(&param) + 1) = uart_wait_get_char();
	*((u8*)(&param) + 2) = uart_wait_get_char();	
	*((u8*)(&param) + 3) = uart_wait_get_char();

	pid_set_d (param);
}

void set_pid_setpoint (void)
{
	u8 byte_l, byte_h;
	u16 setpoint;

	byte_l = uart_wait_get_char();
	byte_h = uart_wait_get_char();
	setpoint = (byte_h << 8) | byte_l;

	pid_set_setpoint (setpoint);
}

#ifdef configSYS_DDS_AD9837
void freq_sweep_AD9837(void)
{
	u32 i;
	long int delay;
	u16 adc_val; 

	dds_AD9837_load_freq();		//set the start frequency
	// sweep and write table
	for (i = 0; i < dds_AD9837_inc_cnt; i++)
	{		

		dds_AD9837_write();
		
		delay = 12500;
		while(delay--){};

		// read adc
		adc_start_conv(ADC_ZAMP);
		adc_val = adc_get_val();

	 	// Send data out
		uart_set_char((adc_val));
		uart_set_char(((adc_val >> 8)));

		// read adc for phase data
		adc_start_conv(ADC_PHASE);
		adc_val = adc_get_val();

	 	// Send data out
		uart_set_char((adc_val));
		uart_set_char(((adc_val >> 8)));

		dds_AD9837_increment(i+1);

	}
}
#endif

#ifdef configSYS_DDS_AD5932
void freq_sweep_dds(void)
{
	u32 i;
	u16 adc_val;
	long int delay;

	//dds_power_up();
	dds_write();
	//dds_set_ctrl();
	for (i = 0; i < dds_inc_cnt; i++)
	{
		// read adc
		adc_start_conv(ADC_ZAMP);
		adc_val = adc_get_val();

	 	// Send data out
		//uart_set_char((u8)(adc_val & 0xFF));
		//uart_set_char((u8)((adc_val >> 8) & 0xFF));
		uart_set_char((adc_val));
		uart_set_char(((adc_val >> 8)));


		// read adc for phase data
		adc_start_conv(ADC_PHASE);
		adc_val = adc_get_val();

	 	// Send data out
		uart_set_char((adc_val));
		uart_set_char(((adc_val >> 8)));

		delay = 12500;
		while(delay--){};

		dds_increment();		
	} 		
}
#endif

#ifdef configMOTOR_PCB
void set_dir(char dirchar)
{
	u8 fail = mtr_set_dir ((dirchar == 'l')?mtr_fwd:mtr_bwd);

	if (!fail){
		uart_write ("o");
	} else {
		uart_write ("f");
	}
}

void set_pw (void)
{
	u8 pw = uart_wait_get_char ();
	u8 set_pw_fail = mtr_set_pw (pw);
	
	if (!set_pw_fail){
		uart_write ("o");
	} else {
		uart_write ("f");
	}
}

void single_pulse(void)
{
	mtr_step ();
	uart_write ("o");
}

void auto_approach (void)
{
	u8 approach_fail;
	u16 setpoint_req, setpoint_error;
	u16 coarse_voltage;

	pid_enable (false);

   	uart_wait_get_bytes ((u8*)(&setpoint_req), 2);
   	uart_wait_get_bytes ((u8*)(&setpoint_error), 2);
	approach_fail = mtr_auto_approach (setpoint_req, setpoint_error);

	coarse_voltage = dac_get_val (DAC_ZOFFSET_COARSE);

	if (!approach_fail){
		uart_set_char ('o');
		uart_set_char((u8)((coarse_voltage) & 0xFF));
		uart_set_char((u8)((coarse_voltage >> 8) & 0xFF));
	} else {
		uart_set_char ('s');
	}
}
#endif // #ifdef configMOTOR_PCB

#ifdef configMEMS_2ACT
void device_calibration (void)
{
	u8 actuator_cal;
	u16 max_voltage;

	actuator_cal = uart_wait_get_char();
	uart_wait_get_bytes ((u8*)(&max_voltage), 2);

	switch (actuator_cal)
	{
		case ('l'):
			calibrate_actuator (&left_act, max_voltage); 
			break;
		case ('r'):
			calibrate_actuator (&right_act, max_voltage);
			break;
		case ('z'):
			calibrate_actuator (&z_act, max_voltage);
			break;
	}
}

void set_scan_wait (void)
{
	u16 nsamples = 0;
	uart_wait_get_bytes ((u8*)(&nsamples), 2);

	z_set_samples (nsamples);

	uart_write ("o");
}									

void configure_scan (void)
{
	// read scan parameters over UART
	u8 temp_buffer [2];
	u16 vmin_line, vmin_scan, vmax, numpts, numlines;

	uart_wait_get_bytes (temp_buffer, 2);
	vmin_line = (temp_buffer[0]) | (temp_buffer[1] << 8);

	uart_wait_get_bytes (temp_buffer, 2);
	vmin_scan = (temp_buffer[0]) | (temp_buffer[1] << 8);

	uart_wait_get_bytes (temp_buffer, 2);
	vmax = (temp_buffer[0]) | (temp_buffer[1] << 8);

	uart_wait_get_bytes (temp_buffer, 2);
	numpts = (temp_buffer[0]) | (temp_buffer[1] << 8);

	uart_wait_get_bytes (temp_buffer, 2);
	numlines = (temp_buffer[0]) | (temp_buffer[1] << 8);

	// return 'o' if successful, 'f' if failed
	if (scan_configure (vmin_line, vmin_scan, vmax, numpts, numlines) == 0) {
		uart_write ("o");
	} else {
		uart_write ("f");
	}
}

void start_scan (void)
{
	scan_start ();
}

void step_scan (void)
{
	scan_step ();
}

#endif 

#ifdef configSYS_PGA_LM1971_PGA4311
void set_pga (void)
{
	u8 pga;
	u8 db;

	// Get pga and pga's db to set
	pga = uart_wait_get_char();
	db = uart_wait_get_char();

	switch (pga)
	{
		case ('z'):
			pga_1ch_set (pga_fine, db);
			break;
		case ('a'):
			pga_1ch_set (pga_dds, db);
			break;
		case ('g'):
			pga_4ch_set (pga_x1, db);
			break;
		case ('h'):
			pga_4ch_set (pga_x2, db);
			break;
		case ('i'):
			pga_4ch_set (pga_y1, db);
			break;
		case ('j'):
			pga_4ch_set (pga_y2, db);
			break;
	}

	uart_write ("o");
}
#endif

/**********************************
		interrupt handlers
**********************************/
void IRQ_Handler(void)  __irq  
{
    u32 IRQSTATUS = 0;

	IRQSTATUS = IRQSTA;	   			// Read off IRQSTA register
	
	// SPI Bus IRQs
	if ((IRQSTATUS & BIT14) == BIT14)
	{
		DDS();
	}

	// Timer 2 IRQ
	if ((IRQSTATUS & BIT4) == BIT4)	//Timer 2 interrupt source
	{
		T2CLRI = 0x55;
		MTR ();
	}
}

void FIQ_Handler(void) __irq
{

	u32 FIQSTATUS = 0;

	FIQSTATUS = FIQSTA;

	if ((FIQSTATUS & BIT3) == BIT3) // Timer1 interrupt source - PID
	{
		PID();
		T1CLRI = 0x55;				// Clear interrupt, reload T1LD
	}

	if ((FIQSTATUS & BIT13) == BIT13)
	{
		// Interrupt caused by hardware RX/TX buffer being full, cleared when
		// RX/TX buffer is read
		UART ();
	}
	
	// TODO: move this to IRQ; Timer 4 FIQs
	if ((FIQSTATUS & BIT6) == BIT6)	//Timer4 interrupt source
	{
		WIRE3();
		T4CLRI = 0x55;
	}
}
