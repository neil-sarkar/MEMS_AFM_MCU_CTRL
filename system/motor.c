#include "motor.h"

#ifdef BOARD_m_assem
	#define MTR_DAT_REG		GP2DAT
	#define MTR_DIR 		BIT22
	#define MTR_PWR 		BIT23
	#define MTR_DIR_DD 		(MTR_DIR<<8)
	#define MTR_PWR_DD 		(MTR_PWR<<8)
#elif defined(BOARD_v2)
	#define MTR_DAT_REG		GP2DAT
	#define MTR_DIR 		BIT22
	#define MTR_PWR 		BIT23
	#define MTR_DIR_DD 		(MTR_DIR<<8)
	#define MTR_PWR_DD 		(MTR_PWR<<8)
#else 
	#error "Motor GPIO not defined"
#endif

// Default number of ticks for approach timer 
// 418 clock ticks --> 41.78 million / 100000 = 10us
#define TMR_DFLT  2926

#define COARSE_SCALE 20
u16 coarse_pw[COARSE_SCALE] = {TMR_DFLT,
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

#define Z_SAMPLES 			8
#define Z_SAMPLE_DELAY 		20
#define COARSE_CHANGE 		0.98f
#define COARSE_SPEED_DFLT 	2
#define COARSE_STEP_DWELL 	40000
#define BWD_STEPS 			32
#define FINE_CHANGE 		0.4f
#define FINE_SPEED_DFLT 	1
#define FINE_STEP_DWELL 	800
#define FINE_Z_MAX_SCALER 	0.2f
#define FINE_Z_STEPS 		100.0f
#define FINE_Z_STEP_DWELL 	2000
#define MTR_DISENGAGE_STEPS 2

static volatile bool step_cmp_flag = false;
static u8 coarse_approach (u16 z_amp_limit);
static u8 fine_approach (u16 z_amp_limit, u16 setpoint, u16 setpoint_error);
static void mtr_disengage (void);

static u8 fine_speed 	= FINE_SPEED_DFLT;
static u8 coarse_speed 	= COARSE_SPEED_DFLT;

/**************************************
	 PUBLIC FUNCTION DEFINITIONS
**************************************/
void mtr_init (void)
{
	// Set as output
	MTR_DAT_REG |= MTR_DIR_DD | MTR_PWR_DD;

	// Set default values
	MTR_DAT_REG &= ~MTR_DIR;
	MTR_DAT_REG |= MTR_PWR;

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
		MTR_DAT_REG &= ~MTR_DIR;
		return 0;
	}
	else if (dir == mtr_fwd)
	{
		MTR_DAT_REG |= MTR_DIR;
		return 0;
	}
	return 1;
}

/* Must be inline...
	...TODO: figure out why this has doesn't work unless inlined */
__inline u8 mtr_step (void)
{
	// Reset timer
	step_cmp_flag = false;
	T0CLRI = 0x55;
	// pwr = gnd
	MTR_DAT_REG &= ~MTR_PWR;	
	// Begin timing
	T0CON |= BIT7;
	while (!step_cmp_flag );
	// disable timer
	T0CON &= ~BIT7;
	// pwr = vcc
	MTR_DAT_REG |= MTR_PWR;
	return 0;
}


u8 mtr_auto_approach (u16 setpoint, u16 setpoint_error)
{
	u8 cmd_fail;
	u16 z_amp_start;
	s32 i = 0;
	volatile u32 wait_time;

	/* Take arbritary but significant number of step backwards
		to ensure we are not close to the tip */
	mtr_set_dir (mtr_bwd);
	mtr_set_pw (coarse_speed);
	for (i = 0; i < BWD_STEPS*10; i++){
		mtr_step ();
		wait_time = FINE_STEP_DWELL;
		while (wait_time--);
	}

	/* Gets initial z-amp */
	for (i = DAC_MAX; i > 0; i --){	
		dac_set_val (DAC_ZOFFSET_COARSE, i);
		wait_time = FINE_Z_STEP_DWELL;
		while (wait_time--);
	}
	wait_time = 800000;
	while (wait_time--);
	adc_start_conv (ADC_ZAMP);
	z_amp_start = adc_get_avgw_val (Z_SAMPLES, Z_SAMPLE_DELAY);

	cmd_fail = coarse_approach (COARSE_CHANGE*z_amp_start);
	if (cmd_fail)
		return cmd_fail;

	cmd_fail = fine_approach (FINE_CHANGE*z_amp_start, setpoint, setpoint_error);
	if (cmd_fail)
		return cmd_fail;	

	mtr_disengage ();

	return 0;
}

bool set_fine_speed (u8 pw)
{
	if (pw < 20) 
	{
		T0LD = coarse_pw[pw];
		T0CLRI = 0x55;
		return true;
	}
	return false;	
}

bool set_coarse_speed (u8 pw)
{
	if (pw < 20) 
	{
		T0LD = coarse_pw[pw];
		T0CLRI = 0x55;
		return true;
	}
	return false;
}

/**************************************
			MOTOR ISR
**************************************/
void mtr_handler (void)
{
	T0CON &= ~BIT7;
	T0CLRI = 0x01;				// Clear the currently active Timer0 Irq
	step_cmp_flag = true;
}


/**************************************
	STATIC FUNCTION DEFINITIONS
**************************************/

/* Fine approach - Approach with small motor steps while scanning
		back and forth with the tip using z-offset voltage. If we get too
		close, step the motor back and then reapproach. Repeat method until
		setpoint value is achieved through adjustment of z-offset. */ 
static u8 fine_approach (u16 z_amp_limit, u16 setpoint, u16 setpoint_error)
{
	bool mov_compl = false;
	u16 z_amp, z_amp_min;
	u16 coarse_max;
	u16 fine_z_speed;
	s32 i;
	volatile u32 wait_time;

	coarse_max = dac_get_limit (DAC_ZOFFSET_COARSE)*FINE_Z_MAX_SCALER;
	fine_z_speed = (coarse_max > FINE_Z_STEPS)?(coarse_max/FINE_Z_STEPS):1;


	mtr_set_pw (fine_speed);
	while (!mov_compl){
		/* Step motor back */
		mtr_set_dir (mtr_bwd);
		for (i = 0; i < BWD_STEPS; i++){
			mtr_step ();
			wait_time = FINE_STEP_DWELL;
			while (wait_time--);
		}

		/* Approach with motor */
		z_amp_min = ADC_MAX;
		mtr_set_dir (mtr_fwd);
		while (z_amp_min > z_amp_limit){
			/* Kill approach if requested */
			if (is_received () && uart_get_char () == BRK_CHAR){
				return 1;
			}
	
			/* Step motor closer */
			mtr_step ();	
			wait_time = FINE_STEP_DWELL;
			while (wait_time--);

			/* Sample by moving tip through range of motion */				
			for (i = coarse_max; i >= 0; i -= fine_z_speed){
				/* Move tip */
				dac_set_val (DAC_ZOFFSET_COARSE, i);
				wait_time = FINE_Z_STEP_DWELL;
				while (wait_time--);

				/* Read bridge voltage */
				adc_start_conv (ADC_ZAMP);
				z_amp = adc_get_avgw_val(Z_SAMPLES, Z_SAMPLE_DELAY);
				if (z_amp < z_amp_min) {
					z_amp_min = z_amp;
				}

				/* If we are within setpoint limits */	
				if (abs(z_amp-setpoint)<=setpoint_error){

					/* Wait for transient voltage change to settle */
					wait_time = 200000;
					while (wait_time--);
					/* Resample to confirm we are within required setpoint */
					adc_start_conv (ADC_ZAMP);
					z_amp = adc_get_avgw_val(Z_SAMPLES, Z_SAMPLE_DELAY);
					if (abs(z_amp-setpoint)<=setpoint_error)
					{
						/* Auto approach is finished */
						return 0;
					}
				}
			}
		}
	}
	return 2;
}

/* Begin coarse approach */
static u8 coarse_approach (u16 z_amp_limit)
{
	u16 z_amp, z_amp_min;
	volatile u32 wait_time;

	z_amp_min = ADC_MAX;
	mtr_set_dir (mtr_fwd);
	mtr_set_pw (coarse_speed);
	while (z_amp_min > z_amp_limit){
		/* Kill auto approach if stop requested */
		if (is_received () && uart_get_char () == BRK_CHAR){
			return 1;
		}

		/* Step motor closer */
		mtr_step ();
		wait_time = COARSE_STEP_DWELL;
		while (wait_time--);

		/* Use thermal coupling between z-actuator and sample
			to determine when to stop coarse approach */
		adc_start_conv (ADC_ZAMP);
		z_amp = adc_get_avgw_val(Z_SAMPLES, Z_SAMPLE_DELAY);
		/* Done to prevent noise from masking tip-sample proximty */
		if (z_amp < z_amp_min) {
			z_amp_min = z_amp;
		}
	}
	return 0;
}

static void mtr_disengage (void)
{
	u32 i = 0;
	mtr_set_dir (mtr_bwd);
	mtr_set_pw (coarse_speed);
	for (i = 0; i < MTR_DISENGAGE_STEPS; i ++){
		mtr_step ();
	}
}
