#include "scan.h"

static us16 buffer [BFR_SIZE];

static Actuator* l_act;
static Actuator* r_act;																								 
static Actuator* z_act;

/* Z-actuator sampling data to be returned over UART */
static struct sample_data {
	us16 num_samples;
	u32 z_off_samples;
	u32 z_amp_samples;
	u32 z_phs_samples;
} z_data;

/* State information about scanning. Used to restart scan
	from last point of scanning */
// TODO: make this better?
static struct scan_params {
	us16 z_samples_req;
	us16 vmin_line;
	us16 vmin_scan;
	us16 vmax;
	us16 numpts;																							
	us16 numlines;
	u32 baseline_points;
	u32 i;
	u32 j;
	u32 k;
	u32 adr;
	dac left_act;
	dac right_act;
} scan_state;

static u8 generate_line (const us16 vmin_line, const us16 vmax, const us16 numpts);
static float get_max_linepwr (const us16 vmin_line, const us16 vmax);

void init_scanner (Actuator* left_act, Actuator* right_act, Actuator* z_act){
	l_act = left_act;
	r_act = right_act;
	z_act = z_act;
}

u8 scan_configure (const us16 vmin_line,
			const us16 vmin_scan,
			const us16 vmax,
			const us16 numpts,
			const us16 numlines)
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
	scan_state.left_act = l_act->out_dac;
	scan_state.right_act = r_act->out_dac;

}

void scan_step ()
{
	const float MAX_LINE_PWR = pwr(l_act,scan_state.vmax) + pwr(l_act,scan_state.vmin_line);
	const float MAX_LINE_VOLT = volt(l_act,MAX_LINE_PWR);
	const float POWER_INC = (MAX_LINE_PWR - pwr(l_act,scan_state.vmin_scan))/scan_state.numlines;

	// temp variables that don't carry state information
	u8 measure_point;
	u32 num_outputted = 0;
	us16 left_dac_val = 0, right_dac_val = 0, nsamples;
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
			measure_point = (buffer[scan_state.j]>=MEASURE_BIT);

			// Scale line voltages stored in flash for largest line power
			left_dac_val = ((buffer[scan_state.j++])&0xFFF)*scale_factor;
			right_dac_val = ((buffer[scan_state.j++])&0xFFF)*scale_factor;

			// Output DAC values to move actuators
			dac_set_val (scan_state.left_act, left_dac_val);
			dac_set_val (scan_state.right_act, right_dac_val);
			
			// Send point measurement if it is sample point
		   	if (measure_point) {

				z_init_sample ();
				for (nsamples = 0; nsamples < scan_state.z_samples_req; nsamples++){
					pid_wait_update ();
					z_sample ();
				}
				z_write_data ();
				num_outputted ++;
				
				if (num_outputted >= SCAN_OUT_SIZE){
					return;
				}
			}
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


void z_init_sample (void)
{
	z_data.num_samples = 0;
	z_data.z_off_samples = 0;
	z_data.z_amp_samples = 0;
	z_data.z_phs_samples = 0;	
}

void z_set_samples (us16 num_samples)
{
	scan_state.z_samples_req = num_samples;
}

us16 z_sample (void)
{
	adc_start_conv (ADC_ZAMP);
	z_data.z_amp_samples += adc_get_val (); 
	
	z_data.z_off_samples += dac_get_val(DAC_ZOFFSET_FINE);

	adc_start_conv(ADC_PHASE);
	z_data.z_phs_samples += adc_get_val();
	
	return (++z_data.num_samples);
}

void z_write_data (void)
{
	us16 z_amp = (us16)z_data.z_amp_samples/z_data.num_samples;
	us16 z_off = (us16)z_data.z_off_samples/z_data.num_samples;
	us16 z_phs = (us16)z_data.z_phs_samples/z_data.num_samples;

	uart_set_char((u8)((z_amp) & 0xFF));
	uart_set_char((u8)((z_amp >> 8) & 0xFF));

	uart_set_char((u8)(z_off & 0xFF));
	uart_set_char((u8)((z_off >> 8) & 0xFF));

	uart_set_char((u8)(z_phs & 0xFF));
	uart_set_char((u8)((z_phs >> 8) & 0xFF));
}

/* 	Generates line of power coefficients given DAC values in *bits*.
	Values are stored in flash beginning at base of BLOCK0.
	
	This function is only run once per scan so performance is
	not of a huge importance. So we can do complicated 
	floating point calculations with minimal changes in overall scan
	time.
*/
static u8 generate_line (const us16 vmin_line,
							const us16 vmax, const us16 numpts){

	// Generate dac output levels for max power isothermal
	const float LINE_PWR = get_max_linepwr (vmin_line, vmax);
	const us16 vmid_l = (us16)volt(l_act, LINE_PWR/2.0);
	const us16 vmid_r = (us16)volt(r_act, LINE_PWR/2.0);

	bool output_val;
	us16 left_volt, right_volt, pts;
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
			left_volt = (us16)i;
			right_volt = (us16)(vmin_line+k);

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
				flash_EraseSector (adr);
				flash_WriteAdr (adr, PAGE_SIZE, (u8*)buffer);				
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
			left_volt = (us16)(vmid_l-k);
			right_volt = (us16)i;
										   
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
				flash_EraseSector (adr);
				flash_WriteAdr (adr, PAGE_SIZE, (u8*)buffer);
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

static float get_max_linepwr (const us16 vmin_line, const us16 vmax)
{
	float left_lean = pwr(l_act, vmin_line) + pwr(r_act, vmax);
	float right_lean = pwr(l_act, vmax) + pwr(r_act, vmin_line);
	return (left_lean > right_lean)?left_lean:right_lean;
}
