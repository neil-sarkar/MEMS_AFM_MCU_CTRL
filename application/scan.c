#include "scan.h"

#define SC_GP_REG		GP2DAT
#define SC_FINE_HZ 		BIT22
#define SC_COARSE_HZ 	BIT23
#define SC_FINE_DD 		(SC_FINE_HZ<<8)
#define SC_COARSE_DD 	(SC_COARSE_HZ<<8)

#define HCLK_HZ			41780000
#define SCAN_DFLT_HZ	30
#define SCAN_DFLT_LD	HCLK_HZ/SCAN_DFLT_HZ

static u16 buffer [BFR_SIZE];

static Actuator* l_act;
static Actuator* r_act; 

volatile scan_params scan_state;

static float get_max_linepwr (const u16 vmin_line, const u16 vmax);

/*************************
 HELPER TIMER FUNCTIONS
*************************/

// periodic, core clock
#define INIT_TMR			T1CON 	= BIT6 + BIT9; \
							FIQEN 	= BIT3;
#define ENABLE_TMR			T1CON 	|= BIT7
#define DISABLE_TMR			T1CON 	&= ~BIT7
#define SET_PRD_TMR			T1LD	= scan_state.step_ld; 
#define LD_START_TMR		T1CLRI 	= 0x01; \
							ENABLE_TIMER

void init_scanner (Actuator* left_act, Actuator* right_act){
	l_act = left_act;
	r_act = right_act;

	// initialize scan parameters
	scan_state.left_act 	= l_act->out_dac;
	scan_state.right_act 	= r_act->out_dac;
	scan_state.freq_hz		= SCAN_DFLT_HZ;
	scan_state.freq_ld		= SCAN_DFLT_LD;
		
	// Set GPIOs as output
	SET_1(SC_GP_REG, SC_FINE_DD | SC_COARSE_DD);

	// Set default values
	SET_0(SC_GP_REG, SC_FINE_HZ);
	SET_0(SC_GP_REG, SC_COARSE_HZ);

	// Initialize scanner timer
	INIT_TMR;
}

u8 scan_configure (const u16 vmin_line,
			const u16 vmin_scan,
			const u16 vmax,
			const u16 numpts)
{
	if (vmin_line > vmax || vmin_scan > vmax)
		return 1;
	scan_state.vmin_line = vmin_line;
	scan_state.vmin_scan = vmin_scan;
	scan_state.vmax = vmax;
	if (numpts == 0 || (numpts & (numpts-1)))
		return 1;
	scan_state.numpts = numpts;

	// Calculates first line and puts it into flash
	return generate_line (vmin_line, vmax, numpts);
}

void scan_reset_state (void)
{
	// restore default state
	scan_state.i = 0;
	scan_state.j = PAGE_SIZE;
	scan_state.k = 0;
	scan_state.adr = BLOCK0_BASE;

	if (scan_state.baseline_points != 0u)
	{
		scan_state.step_ld = scan_state.freq_ld/scan_state.baseline_points;
	} else

	SET_PRD_TMR;
}

void scan_start ()
{
	scan_reset_state ();
	ENABLE_TMR;
}

void scan_set_freq (u8 frequency)
{
	if (frequency != 0)
	{
		scan_state.freq_hz = frequency;
		scan_state.freq_ld = HCLK_HZ/frequency;
	}
}

void scan_handler (void)
{	
	dac swap;

	GP_TOGGLE(SC_GP_REG, SC_FINE_HZ);		
	// value of k is incremented before execution of loop to allow for pausing
	if ((scan_state.k++) < scan_state.baseline_points)
	{
		// Read data stored in flash
		if (scan_state.j >= BFR_SIZE)
		{
			flash_ReadAdr (scan_state.adr, PAGE_SIZE, (u8*)buffer);
			scan_state.adr += PAGE_SIZE;
			scan_state.j = 0;
		}

		// Output DAC values to move actuators
		dac_set_val (scan_state.left_act, ((buffer[scan_state.j++])&0xFFF));
		dac_set_val (scan_state.right_act, ((buffer[scan_state.j++])&0xFFF));
	}
	else 
	{
		GP_TOGGLE(SC_GP_REG, SC_FINE_HZ);
		swap = scan_state.left_act;
		scan_state.left_act = scan_state.right_act;
		scan_state.right_act = swap;
	
		scan_state.adr = BLOCK0_BASE;
		scan_state.j = BFR_SIZE;
		scan_state.k = 0;

		if (uart_get_char() == 'q')
		{	
			DISABLE_TMR;			
		}
	}	
}

/* 	Generates line of power coefficients given DAC values in *bits*.
	Values are stored in flash beginning at base of BLOCK0.
	
	This function is only run once per scan so performance is
	not of a huge importance. So we can do complicated 
	floating point calculations with minimal changes in overall scan
	time.
*/
u8 generate_line (const u16 vmin_line,
							const u16 vmax, const u16 numpts){

	// Generate dac output levels for max power isothermal
	const float LINE_PWR = get_max_linepwr (vmin_line, vmax);
	const u16 vmid_l = (u16)volt(l_act, LINE_PWR/2.0);
	const u16 vmid_r = (u16)volt(r_act, LINE_PWR/2.0);

	bool output_val;
	u16 left_volt, right_volt, pts;
	u32 j = 0, k = 0, adr = BLOCK0_BASE;
	float power = 0;
	float i = 0;
	float step = -1.0f*(vmax-vmid_l)/((float)numpts/2.0f-1.0f);

	// Ensures constant power while actuator values are swept over line
	i = vmax;
	k = 0;
 	for (pts = 0; pts < (numpts/2.0f); pts++)
	{
		output_val = false;
		while (!output_val)
		{
			// DAC value calculations
			left_volt = (u16)i;
			right_volt = (u16)(vmin_line+k);

			buffer[j]= left_volt;
			buffer[j+1]=right_volt;
			// Check power of next interpolation point
			power = pwr(l_act,left_volt) + pwr(r_act,right_volt);

			// This check is necessary when the calibration
			// data for the 2 actuators is verrry different
			if (right_volt >= DAC_MAX)
				return 1; 

			// Write bit to signify z-actuator read point
			if (power >= LINE_PWR){
				buffer [j]|=MEASURE_BIT;
				buffer [j+1]|=MEASURE_BIT;
				output_val = true;
			}

			k += 1;
			j += 2;

			// Write to flash on full buffer
			if (j >= BFR_SIZE)
			{
				if (flash_EraseSector (adr))
					return 1;
				if (flash_WriteAdr (adr, PAGE_SIZE, (u8*)buffer))
					return 1;				
				adr += PAGE_SIZE; 
				j = 0;
			}
		}
		i +=step;
	}
	scan_state.baseline_points = k;

	// Generates the opposite scan pattern. Need to clean this up
	step = 1.0f*(vmax-vmid_r)/((float)numpts/2.0f-1.0f);
	k = 0;
	i = vmid_r;
	for (pts = 0; pts < (numpts/2.0f); pts++)
	{
		output_val = false;
		while (!output_val)
		{				
			left_volt = (u16)(vmid_l-k);
			right_volt = (u16)i;
										   
			buffer [j]=left_volt;
			buffer [j+1]=right_volt;
			power = pwr (l_act,left_volt) + pwr (r_act,right_volt);

			if (vmid_l < k){
				return 1;
			}

			if (power <= LINE_PWR){
				buffer [j]|=MEASURE_BIT;
				buffer [j+1]|=MEASURE_BIT;
				output_val = true;
			}
			
			k += 1;
			j += 2;
			
			if (j >= BFR_SIZE)
			{
				if (flash_EraseSector (adr))
					return 1;
				if (flash_WriteAdr (adr, PAGE_SIZE, (u8*)buffer))
					return 1;
				adr += PAGE_SIZE;
				j = 0;
			}
		}
		i += step;
	}
	scan_state.baseline_points += k;
	
	// Writes whatever is left over in the buffer
	if (j > 0 && j < BFR_SIZE)
	{		
		flash_EraseSector(adr);
		flash_WriteAdr (adr, PAGE_SIZE, (u8*)buffer);		
	}
	
	return 0; 
}

static float get_max_linepwr (const u16 vmin_line, const u16 vmax)
{
	float left_lean = pwr(l_act, vmin_line) + pwr(r_act, vmax);
	float right_lean = pwr(l_act, vmax) + pwr(r_act, vmin_line);
	return (left_lean > right_lean)?left_lean:right_lean;
}
