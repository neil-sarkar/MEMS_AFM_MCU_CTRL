#include "main.h"

#include <stdlib.h>
#include "../peripheral/uart.h"
#include "../peripheral/adc.h"
#include "../peripheral/dac.h"
#include "../peripheral/flash.h"
#include "../peripheral/wire3.h"

#include "../system/pga_1ch.h"
#include "../system/pga_4ch.h"
#include "../system/dds.h"
#include "calibration.h"
#include "scan.h"

tyVctHndlr	  SCAN		= (tyVctHndlr)scan_handler;
tyVctHndlr    DDS     	= (tyVctHndlr)dds_handler;
tyVctHndlr 	  UART		= (tyVctHndlr)uart_handler;
tyVctHndlr	  WIRE3		= (tyVctHndlr)wire3_handler;
extern int dds_inc_cnt;

static Actuator left_act;
static Actuator right_act;

#define SWEEP_MAX 4096

// DAC attenuators CS bits
#define DAC0_CS_DIR_BIT BIT29
#define DAC0_CS_BIT	BIT21

// Macro function to clear terminal screen
#define CLEAR() uart_write("\033c")

// Delay variable for the calibration routine
extern u8 calib_delay;

// coarse approach ISR flag
//volatile bool flag;	
//void set_dir(char);

int main(void)
{
	/*
	 * MCU Initialization				  
	 */
	/* Configure CPU Clock for 41.78MHz, CD=0 */
	POWKEY1 = 0x01;
	POWCON  = 0x00;
	POWKEY2 = 0xF4;

	FIQEN = 0;

	/* Initialize flash memory */
	flash_Init ();

   	/* Initialize SPI/DDS */
	dds_spi_init();

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

	/* Init actuators */
	init_act (&left_act, DAC_Y1, ADC_Y1);
	init_act (&right_act, DAC_X1, ADC_X1);

	init_scanner (&left_act, &right_act);

   	um_init ();

	/*
	 * Main program loop
	 */
	while (true)
	{	
		switch (uart_wait_get_char())
		{
			case 'a':
				write_dac();
				break;
			case 'b':
				read_dac();
				break;
			case 'c':
				read_adc();
				break;
			case 'f':
				set_actuators();
				break;			
			case 'o':
				device_calibration ();
				break;			
			case 'r':
				freq_sweep_dds();
				break;
			case 't':
				dds_increment();
				break;
			case 'u':
				dds_get_data();
				break;
			case 'z':
				adc_set_pga(padc0, uart_wait_get_char());
				break;
			case '@':
				configure_scan ();
				break;
			case '#':
				scan_start ();
				break;
			case '&':
				set_dac_max ();
				break;
			case '*':
				set_pga ();
				break;
			case 'A':
				set_pv_rel_manual_a ();
				break;
			case 'B':
				set_pv_rel_manual_b ();
				break;
			case 'C':
				set_pv_rel_manual_c ();
				break;
			case 'J':
				act_res_test ();
				break;
			case 'M':
				reset_mcu ();
				break;
			case 'O':
				calib_delay = uart_wait_get_char ();
				break;
			case 'm':
				scan_reset_state ();
				break;
			case 'n':
				scan_set_freq (uart_wait_get_char ());
				break;
			case 'N':
				um_sweep();
				break;
			case 'd':
				um_track();
				break;
			case 'e':
				um_set_delta();
				break;
		}
	}
}

#define DAC_1_V		1861

struct umirror
{
	u16 dac_val;
	u16 dac_val_min;
	u16 delta;
	u16 dac_delta;	
};

struct umirror um = {0, 0, 10, 10};

void um_init (void)
{
	adc_set_pga(ADC_MIRROR, 32);
}

void um_set_delta (void)
{
	u8 val_l, val_h;

	val_l 			= uart_wait_get_char();
	val_h 			= uart_wait_get_char();
	um.delta 		= (val_h << 8) | val_l;

	val_l 			= uart_wait_get_char();
	val_h 			= uart_wait_get_char();
	um.dac_delta 	= (val_h << 8) | val_l;
}

#define UM_INC 		um.dac_val += um.dac_delta; 								\
					if (um.dac_val > DAC_1_V)	um.dac_val = DAC_1_V;			\
					dac_set_val(DAC_ZOFFSET_COARSE, um.dac_val);

#define UM_DEC 		if ((um.dac_val - um.dac_delta) < 124) um.dac_val = 124; 	\
					else um.dac_val -= um.dac_delta;							\
					dac_set_val(DAC_ZOFFSET_COARSE, um.dac_val);

void um_track (void)
{
	s32 val, pval;
	s16 dir = 1;

   	// cold-start the algorithm
	adc_start_conv(ADC_MIRROR);
	pval = adc_get_avgw_val(32, 10);
	
	while (COMRX != 'q')
	{
		adc_start_conv(ADC_MIRROR);
		val = adc_get_avgw_val(32, 10);

		if ((abs(val - pval)) > um.delta)
		{
			if (((val - pval)*dir) > 0)
			{
				dir = 1;
				UM_INC;
			}
			else
			{
				dir = -1;
				UM_DEC;
			}
		}
		else
		{
			if (dir == 1)
			{
				UM_INC;
			}
			else if (dir == -1)
			{
				UM_DEC;				
			}		
		}

		uart_set_char (um.dac_val);
		uart_set_char (um.dac_val >> 8);

		uart_set_char (val);
		uart_set_char (val >> 8);
		
		pval = val;	
	}
}

void um_sweep (void)
{
	u16 i;
	u16 val; 
	u16 max = 0;
	u16 min = 0xFFFF;

	// find maximum
	for (i = 0; i < DAC_1_V; i += 20)
	{
		dac_set_val(DAC_ZOFFSET_COARSE, i);
		adc_start_conv(ADC_MIRROR);
		val = adc_get_avgw_val(64, 100);
		if (val > max) 
		{
			max = val;
			um.dac_val = i;
		}
		if (val < min)
		{
			min = val;
		}
	}

	// set voltage to maximum
	dac_set_val(DAC_ZOFFSET_COARSE, um.dac_val);

	// send data
	uart_set_char(max);
	uart_set_char(max >> 8);

	uart_set_char(um.dac_val);
	uart_set_char(um.dac_val >> 8);
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
	}	
}

void set_dac_max (void)
{
	dac dac_ch;
	u16 new_dac_limit;

	// Get DAC channel
	dac_ch = (dac)uart_wait_get_char();
	// Get new DAC limit
	uart_wait_get_bytes((u8*)(&new_dac_limit), 2);

	if (dac_set_limit (dac_ch, new_dac_limit) == 0){
		uart_set_char ('o');
	} else {
		uart_set_char ('f');
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
	}
}									

u16 scan_l_points[1024];
u16 scan_r_points[1024];
void configure_scan (void)
{
	// read scan parameters over UART
	u8 temp_buffer [2];
	u16 numpts, i;

	uart_wait_get_bytes (temp_buffer, 2);
	numpts = (temp_buffer[0]) | (temp_buffer[1] << 8);

	for (i = 0; i < numpts; i++)
	{
		uart_wait_get_bytes (temp_buffer, 2);
		scan_l_points[i]   =  ((temp_buffer[0]) | (temp_buffer[1] << 8)) & 0x0FFF;
	} 

	for (i = 0; i < numpts; i++)
	{
		uart_wait_get_bytes (temp_buffer, 2);
		scan_r_points[i]   =  ((temp_buffer[0]) | (temp_buffer[1] << 8)) & 0x0FFF;
	} 

	scan_configure (numpts);
}

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

void reset_mcu ()
{
	RSTSTA |= BIT2; 	
}

/**********************************
		interrupt handlers
**********************************/
void IRQ_Handler(void)  __irq  
{
    u32 IRQSTATUS = 0;
			
	IRQSTATUS = IRQSTA;	   			// Read off IRQSTA register
	
	// DDS SPI interrupt
	if ((IRQSTATUS & BIT14) == BIT14)
	{
		DDS();
	}

	// Scanner timer interrupt
	if ((IRQSTATUS & BIT3) == BIT3) // Timer1 interrupt source
	{
		T1CLRI = 0x01;				// Clear interrupt, reload T1LD
		SCAN ();
	}
}

void FIQ_Handler(void) __irq
{

	u32 FIQSTATUS = 0;

	FIQSTATUS = FIQSTA;					

	// Timer 4 FIQs
	if ((FIQSTATUS & BIT6) == BIT6)	//Timer4 interrupt source
	{
		WIRE3();
	}

	// UART buffer interrupt
	if ((FIQSTATUS & BIT13) == BIT13)
	{
		// Interrupt caused by hardware RX/TX buffer being full, cleared when
		// RX/TX buffer is read
		UART ();
	}
}
