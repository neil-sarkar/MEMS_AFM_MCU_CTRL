#include "scan.h"

#define SC_GP_REG		GP2DAT
#define SC_FINE_HZ 		BIT22
#define SC_COARSE_HZ 	BIT23
#define SC_FINE_DD 		(SC_FINE_HZ<<8)
#define SC_COARSE_DD 	(SC_COARSE_HZ<<8)

#define HCLK_HZ			41780000
#define SCAN_DFLT_HZ	30
#define SCAN_DFLT_LD	HCLK_HZ/SCAN_DFLT_HZ

static Actuator* l_act;
static Actuator* r_act; 

volatile scan_params scan_state;

/*************************
 HELPER TIMER FUNCTIONS
*************************/

// periodic, core clock

void init_scanner (Actuator* left_act, Actuator* right_act){
	l_act = left_act;
	r_act = right_act;

	// initialize scan parameters
	scan_state.left_act 	= l_act->out_dac;
	scan_state.right_act 	= r_act->out_dac;
	scan_state.freq_hz		= SCAN_DFLT_HZ;
	scan_state.freq_ld		= SCAN_DFLT_LD;
		
	// Set GPIOs as output
	//SET_1(SC_GP_REG, SC_FINE_DD | SC_COARSE_DD);
	GP2DAT = SC_FINE_DD | SC_COARSE_DD;

	// Set default values
	//SET_0(SC_GP_REG, SC_FINE_HZ);
	//SET_0(SC_GP_REG, SC_COARSE_HZ);
	GP2DAT &= ~SC_FINE_HZ;
	GP2DAT &= ~SC_COARSE_HZ;

	// Initialize scanner timer
	T1LD	= SCAN_DFLT_LD;
	T1CON 	|= BIT6 | BIT9;
	IRQEN 	|= BIT3;
}

extern u16 scan_l_points[1024];
extern u16 scan_r_points[1024];
void scan_configure (const u16 numpts)
{
	scan_state.numpts = numpts;
}

void scan_reset_state (void)
{
	T1CON &= ~BIT7;

	// restore default state
	scan_state.i = 0;
	scan_state.j = PAGE_SIZE;
	scan_state.k = 0;
	scan_state.adr = BLOCK0_BASE;

	scan_state.left_act 	= l_act->out_dac;
	scan_state.right_act 	= r_act->out_dac;

	// reset actuators back to their default position
	dac_set_val (scan_state.left_act, 0);
	dac_set_val (scan_state.right_act, 0);

	// TODO: hmmm send a byte back to labview if error?
	if (scan_state.numpts != 0)
	{
		scan_state.step_ld = scan_state.freq_ld/(scan_state.numpts);
	}

	T1LD = scan_state.step_ld;
}
			
void scan_start ()
{
	scan_reset_state ();
	T1CLRI  = 0x55;
	T1CON  |= BIT7;
}

void scan_set_freq (u8 frequency)
{
	if (frequency != 0)
	{
		scan_state.freq_hz = frequency;
		scan_state.freq_ld = HCLK_HZ/frequency;
	}
}

volatile u16 s_i = 0;
volatile u8 dir = 1;

void scan_handler (void)
{
   	if ((s_i == (scan_state.numpts-1)) || (s_i == 0))
	{
		GP2DAT ^= SC_COARSE_HZ;
		if (COMRX == 'q')
			scan_reset_state ();
	}
	
	GP2DAT ^= SC_FINE_HZ;
	DAC7DAT = (scan_l_points[s_i] << 16);		 
	DAC10DAT = (scan_r_points[s_i] << 16);

	if (dir == 1)
	{
		s_i++;
		if (s_i == (scan_state.numpts-1)) 
			dir = 0;
	}
	else
	{
		s_i--;
		if (s_i == 0)
			dir = 1;
	}	
}
