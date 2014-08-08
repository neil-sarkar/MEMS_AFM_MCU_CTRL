#include "main.h"

#include "../peripheral/uart.h"
#include "../peripheral/adc.h"
#include "../peripheral/dac.h"
#include "../peripheral/flash.h"
#include "../peripheral/pga_1ch.h"
#include "../peripheral/pga_4ch.h"
#include "../peripheral/flash.h"

#include "../system/dds.h"
#include "../system/pid.h"
#include "../system/filter.h"
#include "../system/motor.h"
#include "../system/wire3.h"
#include "calibration.h"
#include "scan.h"

tyVctHndlr    DDS     	= (tyVctHndlr)dds_handler;
tyVctHndlr    FILTER   	= (tyVctHndlr)filter_handler;
tyVctHndlr    PID     	= (tyVctHndlr)pid_handler;
tyVctHndlr 	  UART		= (tyVctHndlr)uart_handler;
tyVctHndlr	  MTR		= (tyVctHndlr)mtr_handler;
extern int dds_inc_cnt;

static Actuator left_act;
static Actuator right_act;
static Actuator z_act;

#define SWEEP_MAX 4096

// DAC attenuators CS bits
#define DAC0_CS_DIR_BIT BIT29
#define DAC0_CS_BIT	BIT21

// Macro function to clear terminal screen
#define CLEAR() uart_write("\033c")

// coarse approach ISR flag
//volatile bool flag;	
//void set_dir(char);

int main(void)
{
	u8 rx_char;

	/*
	 * MCU Initialization				  
	 */
	/* Configure CPU Clock for 41.78MHz, CD=0 */
	POWKEY1 = 0x01;
	POWCON  = 0x00;
	POWKEY2 = 0xF4;

	/* Initialize flash memory */
	flash_Init ();

   	/* Initialize SPI/DDS */
	dds_spi_init();
	//dds_power_up();

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
	mtr_init ();

	/* Init DAC attenuators */
	pga_1ch_init (pga_fine);
	pga_1ch_init (pga_dds);
	pga_4ch_init ();		   

	pga_4ch_set (pga_x1, 0);
	pga_4ch_set (pga_x2, 0);
	pga_4ch_set (pga_y1, 0);
	pga_4ch_set (pga_y2, 0);

	/* Init actuators */
	init_act (&left_act, DAC_Y1, ADC_Y1, ADC_ZOFFSET);
	init_act (&right_act, DAC_Y2, ADC_Y2, ADC_ZOFFSET);
	init_act (&z_act, DAC_ZOFFSET_FINE, ADC_ZOFFSET, ADC_Y1);

	init_scanner (&left_act, &right_act, &z_act);
	
	/* Disable filter and PID */
	filter_enable(false);
	pid_enable(false);

	/*
	 * Main program loop
	 */
	while (true)
	{
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
			case 'e':
				read_z();
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
			case 'n':
				cont_pulse();
				break;
			case 'o':
				device_calibration ();
				break;			
			case 'q':
				freq_sweep();
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
			case 'v': 
				auto_approach();
				break;
			case 'w':
				//filter_adc();
				break;
			case 'x':
				filter_enable(true);
				break;
			case 'y':
				filter_enable(false);
				break;
			case 'z':
				adc_set_pga(padc0, uart_wait_get_char());
				break;
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
			case '&':
				set_dac_max ();
				break;
			case '*':
				set_pga ();
				break;
		}
	}
}

void set_dac_max (void)
{
	dac dac_ch;
	us16 new_dac_limit;

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
	unsigned short dac_val;

	// Get DAC channel
	dac_ch = (dac)uart_wait_get_char();
	
	// Get DAC value
	dac_val = dac_get_val(dac_ch);

	uart_set_char(dac_val & 0xFF);
	uart_set_char((dac_val >> 8) & 0xFF);		
}

void read_z (void)
{
	z_init_sample ();
	pid_wait_update ();
	z_sample ();
	z_write_data ();
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
	dac_set_val (DAC_Y2, ((dac1_h << 8) | dac1_l) & 0x0FFF);
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

void freq_sweep(void)
{
	u16 i, val;
	u16 steps;
	u16 diff;
	u16 adc_val;
	u8 steps_l;
	u8 steps_h;
	//u8 sweep[SWEEP_MAX * 2];
	long int delay;

	steps_l = uart_wait_get_char();
	steps_h = uart_wait_get_char();

	steps = ((steps_h << 8)| steps_l) & 0x0FFF;

	diff = SWEEP_MAX / steps;
	
	// sweep and write table
	for (i = 0, val = 0; i < (steps*2); i += 2, val += diff)
	{
		dac_set_val(DAC_ZVCO, val);

		// delay is about ~4.60ms
		delay = 20000;
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

	}
}

void freq_sweep_dds(void)
{
	u32 i;
	us16 adc_val;
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

		delay = 12500;
		while(delay--){};

		dds_increment();		
	} 		
}

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

void cont_pulse(void)
{
	for (;;) {
		/* Kill approach if requested */
		if (is_received () && uart_get_char () == BRK_CHAR){
			break;
		}

		mtr_step ();
	}
	uart_write ("o");
}

void auto_approach (void)
{
	u8 approach_fail;
	us16 setpoint_req, setpoint_error;
	us16 coarse_voltage;

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

void device_calibration (void)
{
	u8 actuator_cal;
	us16 max_voltage;

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
	us16 nsamples = 0;
	uart_wait_get_bytes ((u8*)(&nsamples), 2);

	z_set_samples (nsamples);

	uart_write ("o");
}									

void configure_scan (void)
{
	// read scan parameters over UART
	us16 vmin_line, vmin_scan, vmax, numpts, numlines;

	uart_wait_get_bytes ((u8*)(&vmin_line), 2);

   	uart_wait_get_bytes ((u8*)(&vmin_scan), 2);

   	uart_wait_get_bytes ((u8*)(&vmax), 2);

	uart_wait_get_bytes ((u8*)(&numpts), 2);

	uart_wait_get_bytes ((u8*)(&numlines), 2);

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

void IRQ_Handler(void)  __irq  
{
    u32 IRQSTATUS = 0;

	IRQSTATUS = IRQSTA;	   			// Read off IRQSTA register
	
	// SPI Bus IRQs
	if ((IRQSTATUS & BIT14) == BIT14)
	{
		DDS();
	}

	// Timer 0 IRQs
	if ((IRQSTATUS & BIT2) == BIT2)	//Timer 0 interrupt source
	{
		MTR ();
	}

	// Timer 4 IRQs
	if ((IRQSTATUS & BIT6) == BIT6)	//Timer 0 interrupt source
	{
		wire3_handler ();
	}
}

void FIQ_Handler(void) __irq
{

	u32 FIQSTATUS = 0;

	FIQSTATUS = FIQSTA;

	if ((FIQSTATUS & BIT3) == BIT3) // Timer1 interrupt source - PID
	{
		PID();
		T1CLRI = 0x01;				// Clear interrupt, reload T1LD
	}

	if ((FIQSTATUS & BIT4) == BIT4) // Timer4 interrupt source - PID
	{
		//GP0DAT |= BIT23;
		FILTER();
		T2CLRI = 0x01;				// Clear interrupt, reload T1LD
		//GP0DAT &= ~BIT23;
	}					

	if ((FIQSTATUS & BIT13) == BIT13)
	{
		UART ();
		// Interrupt caused by RX/TX buffer being full, cleared when
		// RX/TX buffer is read
	}
}
