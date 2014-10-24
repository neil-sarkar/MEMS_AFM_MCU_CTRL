#include "scan.h"

#define SC_GP_REG		GP2DAT
#define SC_FINE_HZ 		BIT22
#define SC_COARSE_HZ 	BIT23
#define SC_FINE_DD 		(SC_FINE_HZ<<8)
#define SC_COARSE_DD 	(SC_COARSE_HZ<<8)

static u16 buffer [BFR_SIZE];

static Actuator* l_act;
static Actuator* r_act; 

scan_params scan_state;

static float get_max_linepwr (const u16 vmin_line, const u16 vmax);

void init_scanner (Actuator* left_act, Actuator* right_act){
	l_act = left_act;
	r_act = right_act;

	// Set GPIOs as output
	SET_1(SC_GP_REG, SC_FINE_DD | SC_COARSE_DD);

	// Set default values
	SET_0(SC_GP_REG, SC_FINE_HZ);
	SET_0(SC_GP_REG, SC_COARSE_HZ);

	// Initialize scanner timer
//	T1LD  = TMR_DFLT;
	T1CON = BIT6 + BIT9;						// Periodic mode, core clock
	FIQEN = BIT3;								// Enable Timer2 fast interrupt
}

u8 scan_configure (const u16 vmin_line,
			const u16 vmin_scan,
			const u16 vmax,
			const u16 numpts,
			const u16 numlines)
{
	if (vmin_line > vmax || vmin_scan > vmax)
		return 1;
	scan_state.vmin_line = vmin_line;
	scan_state.vmin_scan = vmin_scan;
	scan_state.vmax = vmax;
	if (numpts == 0 || (numpts & (numpts-1)))
		return 1;
	scan_state.numpts = numpts;
	scan_state.numlines = numlines;

	// Calculates first line and puts it into flash
	return generate_line (vmin_line, vmax, numpts);
}

void scan_start ()
{
	// restore default state
	scan_state.i = 0;
	scan_state.j = PAGE_SIZE;
	scan_state.k = 0;
	scan_state.adr = BLOCK0_BASE;
	// TODO: what values are the lateral actuators being set to?
	scan_state.left_act = l_act->out_dac;
	scan_state.right_act = r_act->out_dac;

}

void scan_step ()
{
	const float MAX_LINE_PWR = pwr(l_act,scan_state.vmax) + pwr(l_act,scan_state.vmin_line);
	const float MAX_LINE_VOLT = volt(l_act,MAX_LINE_PWR);
	const float POWER_INC = (MAX_LINE_PWR - pwr(l_act,scan_state.vmin_scan))/scan_state.numlines;

	u16 left_dac_val = 0, right_dac_val = 0;
	float scale_factor = MAX_LINE_VOLT;
	dac swap;

	for (; scan_state.i < scan_state.numlines*2; scan_state.i ++)
	{
		scale_factor = volt(l_act,MAX_LINE_PWR - POWER_INC*(scan_state.i/2))/MAX_LINE_VOLT;
		
		// value of k is incremented before execution of loop to allow for pausing
		while ((scan_state.k++) < scan_state.baseline_points)
		{
			// Read data stored in flash
			if (scan_state.j >= BFR_SIZE)
			{
				flash_ReadAdr (scan_state.adr, PAGE_SIZE, (u8*)buffer);
				scan_state.adr += PAGE_SIZE;
				scan_state.j = 0;
			}

			// Scale line voltages stored in flash for largest line power
			left_dac_val = ((buffer[scan_state.j++])&0xFFF)*scale_factor;
			right_dac_val = ((buffer[scan_state.j++])&0xFFF)*scale_factor;

			// Output DAC values to move actuators
			dac_set_val (scan_state.left_act, left_dac_val);
			dac_set_val (scan_state.right_act, right_dac_val);
		}

		swap = scan_state.left_act;
		scan_state.left_act = scan_state.right_act;
		scan_state.right_act = swap;

		scan_state.adr = BLOCK0_BASE;
		scan_state.j = BFR_SIZE;
		scan_state.k = 0;
	}

	dac_set_val (scan_state.left_act, 0);
	dac_set_val (scan_state.right_act, 0);	
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

void scan_handler (void)
{
		
}

static float get_max_linepwr (const u16 vmin_line, const u16 vmax)
{
	float left_lean = pwr(l_act, vmin_line) + pwr(r_act, vmax);
	float right_lean = pwr(l_act, vmax) + pwr(r_act, vmin_line);
	return (left_lean > right_lean)?left_lean:right_lean;
}
