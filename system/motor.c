#include "motor.h"

// Default number of ticks for approach timer 
// 418 clock ticks --> 41.78 million / 100000 = 10us
#define TMR_DFLT  2926

#define COARSE_SCALE 20
unsigned short coarse_pw[COARSE_SCALE] = {TMR_DFLT,
								TMR_DFLT*2,
								TMR_DFLT*3,
								TMR_DFLT*4,
								TMR_DFLT*5,
								TMR_DFLT*6,
								TMR_DFLT*7,
								TMR_DFLT*8,
								TMR_DFLT*9,
								TMR_DFLT*10,
								TMR_DFLT*11,
								TMR_DFLT*12,
								TMR_DFLT*13,
								TMR_DFLT*14,
								TMR_DFLT*15,
								TMR_DFLT*16,
								TMR_DFLT*17,
								TMR_DFLT*18,
								TMR_DFLT*19,
								TMR_DFLT*20};

static bool step_cmp_flag = false;

void mtr_init (void)
{
	// Set as output
	GP2DAT |= MOTOR_DIR_DD + MOTOR_PWR_DD;

	// Set default values
	GP2DAT &= ~MOTOR_DIR;
	GP2DAT |= MOTOR_PWR;

	/* Initilize Timer 0 for the coarse approach */
	T0LD  = TMR_DFLT;
	T0CON = BIT6 + BIT9 + BIT10;			// Periodic mode, HCLK core clock 41.78Mhz
    IRQEN = BIT2;							// Enable Timer 0 IRQ
}

u8 mtr_set_pw(u8 pw)
{
	if (pw < 20) 
	{
		T0LD = coarse_pw[pw];
		T0CLRI = 0x55;
		return 0;
	}
	return 1;
}

u8 mtr_set_dir (mtrdir dir)
{
	if (dir == mtr_bwd) 
	{
		GP2DAT &= ~MOTOR_DIR;
		return 0;
	}
	else if (dir == mtr_fwd)
	{
		GP2DAT |= MOTOR_DIR;
		return 0;
	}
	return 1;
}

u8 mtr_step (void)
{
	// Reset timer
	step_cmp_flag = false;
	T0CLRI = 0x55;
	// pwr = gnd
	GP2DAT &= ~MOTOR_PWR;	
	// Begin timing
	T0CON |= BIT7;
	while (!step_cmp_flag );
	// disable timer
	T0CON &= ~BIT7;
	// pwr = vcc
	GP2DAT |= MOTOR_PWR;
	return 0;
}

__inline static void _mtr_step (void)
{
	// Reset timer
	step_cmp_flag = false;
	T0CLRI = 0x55;
	// pwr = gnd
	GP2DAT &= ~MOTOR_PWR;	
	// Begin timing
	T0CON |= BIT7;
	while (!step_cmp_flag );
	// disable timer
	T0CON &= ~BIT7;
	// pwr = vcc
	GP2DAT |= MOTOR_PWR;
}


u8 mtr_auto_approach (void)
{
	const us16 FINE_Z_STEP = DAC_MAX/64;
	const float COARSE_CHANGE = 0.95f;
	const us16 COARSE_SPEED = 2;
	const float FINE_CHANGE = 0.6f;
	const us16 FINE_SPEED = 1;
	const us16 SAMPLES = 8;
	const us16 BWD_STEPS = 64;

	volatile u32 set_time = 200000;
	us16 z_amp, z_amp_min, z_amp_start;
	s32 i;
	us16 z_off_max = dac_get_limit (DAC_ZOFFSET);
	bool mov_compl = false;
	bool mov_fwd_compl = false;
	us16 setpoint = 564;
	us16 setpoint_error = 30;

	for (i = DAC_MAX; i > 0; i --){	
		dac_set_val (dac11, i);
		set_time = 2000;
		while (set_time--);
	}

	set_time = 500000;
	while (set_time--);

	adc_start_conv (ADC_ZAMP);
	z_amp_start = adc_get_avgw_val (SAMPLES, 20);

	mtr_set_dir (mtr_fwd);
	mtr_set_pw (COARSE_SPEED);

	z_amp_min = z_amp_start;
	while (z_amp_min > COARSE_CHANGE*z_amp_start){
		if (is_received () && uart_get_char () == 's'){
			return 1;
		}

		// Step motor closer
		_mtr_step ();

		set_time = 2000;
		while (set_time--);

		adc_start_conv (ADC_ZAMP);
		z_amp = adc_get_avgw_val(SAMPLES, 20);

		if (z_amp < z_amp_min) {
			z_amp_min = z_amp;
		}

		// Write some z-amplitude data
		uart_write_bytes ((u8*)(&z_amp_min), 2);
	}

	mtr_set_pw (FINE_SPEED);

	while (!mov_compl){
		mtr_set_dir (mtr_bwd);
		for (i = 0; i < BWD_STEPS; i++){
			_mtr_step ();
			set_time = 2000;
			while (set_time--);
		}
		
		mtr_set_dir (mtr_fwd);
		mov_fwd_compl = false;
		z_amp_min = z_amp_start;
		while (z_amp_min > FINE_CHANGE*z_amp_start){
			if (is_received () && uart_get_char () == 's'){
				return 1;
			}
	
			// Step motor closer
			_mtr_step ();
	
			set_time = 2000;
			while (set_time--);
	
			for (i = DAC_MAX; i > 0; i -= FINE_Z_STEP){
				dac_set_val (dac11, i);
			
				set_time = 2000;
				while (set_time--);
				
				adc_start_conv (ADC_ZAMP);
				z_amp = adc_get_avgw_val(SAMPLES, 20);
	
				if (z_amp < z_amp_min) {
					z_amp_min = z_amp;
				}
	
				if (abs(z_amp-setpoint)<=setpoint_error){
					set_time = 200000;
					while (set_time--);
					adc_start_conv (ADC_ZAMP);
					z_amp = adc_get_avgw_val(SAMPLES, 20);
					if (abs(z_amp-setpoint)<=setpoint_error)
					{
						mov_compl = true;
						/* TODO change this */
						return 1;
					}
				}
			}
		}
	}

	/*	-- Tests actuation of z
		while (1){
		dac_set_val (dac9, 500);
		for (i = DAC_MAX; i > 0; i --){
			dac_set_val (dac11, i);
			
			adc_start_conv (ADC_ZAMP);
			z_amp = adc_get_avgw_val(SAMPLES, 20);
		}
		dac_set_val (dac9, 0);
		for (i = 0; i <= DAC_MAX; i ++){
			dac_set_val (dac11, i);
			
			adc_start_conv (ADC_ZAMP);
			z_amp = adc_get_avgw_val(SAMPLES, 20);
		}
	} */
			
	uart_write_bytes ((u8*)(&z_amp_min), 2);		

	return 0;
}

void mtr_handler (void)
{
	T0CON &= ~BIT7;
	T0CLRI = 0x55;				// Clear the currently active Timer0 Irq
	step_cmp_flag = true;
}
