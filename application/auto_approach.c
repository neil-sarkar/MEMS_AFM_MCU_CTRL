#include "auto_approach.h"
#include "../system/stpr_DRV8834.h"

// Default number of ticks for approach timer 
#define TMR_DFLT  5000

#define UART_ECHO(ack_char)	uart_set_char(ack_char)

/*
	A generic, motor-independent, auto approach alogirthm 
*/

struct auto_approach 
{
	u8 state;
	u16 setpoint;
	u16 measurement_init;
	u16 measurement;
	u16 timer_count;
};

struct auto_approach aappr;

void auto_approach_begin(void)
{
	u8 msg_id_char, msg_tag_char;
	aappr.state = 1;
	aappr.timer_count = 0;
	uart_wait_get_bytes((u8*)(&aappr.setpoint), 2);
	UART_ECHO('\n');
	
	while(aappr.state != 0){
		//Message processing sequence from main.c need to be duplicated here
		//to take care of the abort signal
		msg_tag_char = uart_get_char();
		msg_id_char = uart_get_char();
		// Process the incoming character
		if (msg_tag_char == '\n'){
			msg_tag_char = '\0';
			msg_id_char = '\0';
		} 
		switch(msg_id_char){
			case 0x9b:
				//return the message tag
				UART_ECHO(msg_tag_char);
				//return message id
				UART_ECHO(0x9b);
				aappr.state=0;
				UART_ECHO('\n');
			break;
			case 0x9c:
				//return the message tag
				UART_ECHO(msg_tag_char);
				//return message id
				UART_ECHO(0x9c);
				auto_approach_get_info();
				UART_ECHO('\n');
			break;
		}
		auto_approach_state_machine();
	}
}

void auto_approach_count(void)
{
	aappr.timer_count++;
}

void auto_approach_get_info(void){
	uart_write_char(aappr.state);
	uart_write_char((aappr.measurement));
	uart_write_char(((aappr.measurement >> 8)));
}


/**
The general 1-ms timer, timer1, will call this function every ms.
To introduce a delay, simply count up on the aappr.timer_count variable until
desired ms is reached. 
*/

void auto_approach_state_machine(){
	switch(aappr.state) {
    case 0: //Disabled state or Aborted state... safe shutdown
			stpr_exit_cont();
      stpr_sleep();
			aappr.state = 0;
				//Do nothing
        break;
    case 1: //Wake up and intialization.
				//Clear the vars
				aappr.measurement = 0;
				aappr.measurement_init = 0;
				aappr.timer_count = 0;
        //Turn OFF PID
				pid_enable(false);
        //Stop and sleep the motor
        //stpr_exit_cont();
				//stpr_sleep();
        //Now wake it up
				stpr_wake();
				stpr_set_dir('f'); //Forward is down...
				stpr_set_speed(0x66BC);	//Very fast
				stpr_set_step(STEP_FULL);
		    stpr_cont(); //Back-up a bit
        aappr.state++;
				//callback required here
				aappr.timer_count = 0;
        break;
    case 2: 
				//200 millseconds of moving down...
				if(aappr.timer_count<200){
					//do nothing
				} else {
					aappr.state++;
				}
        break;
    case 3: //Get initial measurement
				//Stop, switch direction, slow down a bit
        stpr_exit_cont();
				stpr_set_dir('b'); 
				stpr_set_speed(0x647D);	//A bit slower..
        //Measure Signal... ADC_ZOFFSET
        //Clear existing first
        aappr.measurement_init = 0;
        //Read the ADC #5
				// Read ADC, save measurement_init and proceed.
				adc_start_conv(0x05);
				aappr.measurement_init = adc_get_val(); 
        aappr.state++;
				aappr.timer_count = 0;
				break;
		case 4: //Hold for 600msec
				if(aappr.timer_count<600){
					//do nothing
				} else {
					aappr.state++;
				}
        break;
    case 5: //High-speed operation
				//Get data from ADC
				adc_start_conv(0x05);
				aappr.measurement = adc_get_val(); //adc_get_val() apparantely has race condition... to fix
        // The callback for readADC ADCZOFFSET should update autoappr_measurement
				if(aappr.measurement <= aappr.setpoint) {
						//Make sure that setpoint is OK.
						aappr.state = 9;
						break;
				} else {
						// VERY IMPORTANT We also double check that the target is less than 95% of the init to begin with.
						// If it is more than 95%, then we should skip straight to state 6.
						if(aappr.setpoint <= (0.90*aappr.measurement_init)) {
								stpr_cont();
								aappr.state++;
								aappr.timer_count = 0;
						} else {
								aappr.state = 6;
						}
				}
        break;
    case 6: //High speed up, Abort available here.
        // Get measured signal. 
        // If measured signal is less than whatever, do whatever
        // if not yet at .95 init, etc...
        // Actual logic
        // Now check measurement against target value.
        if((aappr.measurement <= (0.95*aappr.measurement_init)) && aappr.measurement > 0) {
            aappr.state++;
            aappr.timer_count = 0;
        } else {
            //Continue running the motor
            //Measure Signal... from ADC_ZOFFSET
            //task1_timer will keep calling the state machine
            adc_start_conv(0x05);
						aappr.measurement = adc_get_val();
        }
        break;
    case 7: //Reduce speed
				stpr_set_step(STEP_8);
				stpr_set_speed(0x4e20); //equivalent to 20000
				adc_start_conv(0x05);
				aappr.measurement = adc_get_val();
        stpr_cont();
        aappr.state++;
    case 8://Abort available here.
        //Loop like state #5
        // Check if measurement is at setpoint autoappr_setpoint
        if((aappr.measurement <= aappr.setpoint) && aappr.measurement > 0) {
            stpr_exit_cont();
            aappr.state++;
        } else {
            //Continue running the motor
            //Measure Signal... from ADC_ZOFFSET
            adc_start_conv(0x05);
						aappr.measurement = adc_get_val();
        }
        break;
    case 9: 
        stpr_exit_cont();
        stpr_sleep();
        //Turn on PID
        pid_enable(true);
        //Clean Up
        aappr.state = 10;
        break;
	}
}

