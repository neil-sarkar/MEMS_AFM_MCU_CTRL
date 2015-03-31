#include "calibration.h"

#ifdef configMEMS_2ACT

#define CALC_COEFF_0(a, b) 		(-0.5f*b/a)
#define CALC_COEFF_1(a, b, c) 	((b*b-4.0f*a*c)/4.0f/(a*a))
#define CALC_COEFF_2(a)			(1.0f/a)

// local function definition	
static void set_pv_rel (Actuator* act, float a, float b, float c);
static void set_indirect_rel (Actuator* act, float a, float b, float c);

extern scan_params scan_state;
u8 calib_delay = 1;		// 25us

/* Function converts power to voltage (in DAC bits).
	Hides calculation so changes can be made to P-V relationship */
float pwr (Actuator* act, float volt){	// assuming integer voltage is minimally affected by rounding voltage
	#if NUM_COEFF == 3
	return act->pv_rel[0]*volt*volt + act->pv_rel[1]*volt + act->pv_rel[2];
	#else
	return volt*volt; 
	#endif
}

/* Function converts voltage  to power (rounded to nearest DAC value).
	Hides calculation so changes can be made to V-P  relationship */
float volt (Actuator* act, float pwr){
	#if NUM_COEFF == 3
	return (act->vp_rel[0]+sqrt(act->vp_rel[1]+act->vp_rel[2]*pwr));
	#else
	return (sqrt(pwr));
	#endif
}

void set_pv_rel_a (Actuator* act, float a)
{
	act->pv_rel[0] = a;
	act->vp_rel[0] = CALC_COEFF_0(a, act->pv_rel[1]);
	act->vp_rel[1] = CALC_COEFF_1(a, act->pv_rel[1], act->pv_rel[2]);
	act->vp_rel[2] = CALC_COEFF_2(a);
	generate_line (scan_state.vmin_line, scan_state.vmax, scan_state.numpts);		
}

void set_pv_rel_b (Actuator* act, float b)
{
	act->pv_rel[1] = b;
	act->vp_rel[0] = CALC_COEFF_0(act->pv_rel[0], b);
	act->vp_rel[1] = CALC_COEFF_1(act->pv_rel[0], b, act->pv_rel[2]);
	generate_line (scan_state.vmin_line, scan_state.vmax, scan_state.numpts);		
}

void set_pv_rel_c (Actuator* act, float c)
{
	act->pv_rel[2] = c;
	act->vp_rel[1] = CALC_COEFF_1(act->pv_rel[0], act->pv_rel[1], c);
	generate_line (scan_state.vmin_line, scan_state.vmax, scan_state.numpts);		
}

void init_act (Actuator* act, dac out_dac, adc in_adc, adc z_adc)
{
	act->out_dac = out_dac;
	act->in_adc = in_adc;
	act->z_adc = z_adc;
} 

void calibrate_actuator (Actuator* act, u16 max_voltage){
	u8 buffer [12];
	u16 i = 0;
	u32 j = 0, adc_val = 0;

	for (i = 0; i < max_voltage; i++)
	{
		// Ramp the voltage
		dac_set_val (act->out_dac, i);

		// Delay
		DELAY_25_US(calib_delay);

		// Measures differential voltage across current sense resistor
		// Averaging loop for lateral actuator TCR measurement and send it across UART
		adc_val = 0;
		for (j = 0; j < ADC_SAMPLES; j ++)
		{
			adc_start_conv(act->in_adc);
			adc_val += adc_get_val ();	
		}
		adc_val /= ADC_SAMPLES;				
		uart_set_char ((u8)(adc_val&0xFF));
		uart_set_char ((u8)((adc_val>>8)&0xFF));

		// Measurment of z-coupling with lateral actuator ramp
		adc_val = 0;
		for (j = 0; j < ADC_SAMPLES; j++){
			adc_start_conv (act->z_adc);
			adc_val += adc_get_val();
		}
		adc_val /= ADC_SAMPLES;
		uart_set_char ((u8)(adc_val&0xFF));
		uart_set_char ((u8)((adc_val>>8))&0xFF);
	}

	dac_set_val (act->out_dac, 0);

	// Get fitted polynomial coefficients
	for (i = 0; i < NUM_COEFF*4; i++)
	{
		buffer [i] = uart_wait_get_char ();
	}

	// Convert to single precision floating point
	set_pv_rel (act, ((float*)buffer)[0],((float*)buffer)[1],((float*)buffer)[2]);

	// Get fitted polynomial coefficients
	for (i = 0; i < NUM_COEFF*4; i++)
	{
		buffer [i] = uart_wait_get_char ();
	}

	// Convert to single precision floating point
	set_indirect_rel (act, ((float*)buffer)[0],((float*)buffer)[1],((float*)buffer)[2]);
}

/************************** 
LOCAL FUNCTION DEFINITIONS
**************************/

static void set_pv_rel (Actuator* act, float a, float b, float c)
{
	act->pv_rel [0] = a;
	act->pv_rel [1] = b;
	act->pv_rel [2] = c;
	
	act->vp_rel[0] = CALC_COEFF_0(a, b);
	act->vp_rel[1] = CALC_COEFF_1(a, b, c);
	act->vp_rel[2] = CALC_COEFF_2(a);
}

// TODO: WHAT IS THE POINT OF THIS IF IT IS NEVER USED?
static void set_indirect_rel (Actuator* act, float a, float b, float c)
{
	act->rv_indirect_rel [0] = a;
	act->rv_indirect_rel [1] = b;
	act->rv_indirect_rel [2] = c;
	
	act->vr_indirect_rel[0] = CALC_COEFF_0(a, b);
	act->vr_indirect_rel[1] = CALC_COEFF_1(a, b, c);
	act->vr_indirect_rel[2] = CALC_COEFF_2(a);
}

#endif
