#include "auto_approach.h"
#include "../system/stpr_DRV8834.h"

// Default number of ticks for approach timer 
#define TMR_DFLT  5000

#define UART_ECHO(ack_char)	uart_set_char(ack_char)

/*
	A generic, motor-independent, auto approach alogirthm 
*/

enum auto_approach_states
{
	DISABLED=0,
	WAKEUP,
	FAST_RETR_GO,
	FAST_RETR_STOP,
	INIT_MEAS,
	PRE_FAST_APPR,
	FAST_APPR,
	PRE_SLOW_APPR,
	SLOW_APPR,
	SETPOINT_REACHED
};

struct auto_approach 
{
	enum auto_approach_states state;
	u16 setpoint;
	u16 measurement_init;
	u16 measurement;
	u16 timer_count;
};


u8 msg_char;
u16 timer_timestamp_1, timer_timestamp_2;

struct auto_approach aappr;

void auto_approach_begin(void)
{
//	u16 init_tmr_count;
	aappr.state = WAKEUP;
	aappr.timer_count = 0;
	uart_wait_get_bytes((u8*)(&aappr.setpoint), 2);
	UART_ECHO('\n');
	
	while(aappr.state != DISABLED){
		// Run the actual state machine
		auto_approach_state_machine();
		//Give 1ms to process any incoming UART
		timer_timestamp_1 = aappr.timer_count;
		while(aappr.timer_count - timer_timestamp_1 <= 1){
			if(!auto_approach_serial_read()){
				return;
			}
		}
		// Every 300ms we send a status update
		if((aappr.state == FAST_APPR || aappr.state == SLOW_APPR) && (abs(aappr.timer_count - timer_timestamp_2) > 200)){
			timer_timestamp_2 = aappr.timer_count;
			auto_approach_send_info();
		}
	}
}

bool auto_approach_serial_read(void)
{
	//listen for the magic 0x9b char, which means stop auto approach
	msg_char = uart_get_char();

	// Process the incoming character
	if (msg_char == 0x9b){
		aappr.state=DISABLED;
		UART_ECHO('\n');
		UART_ECHO(0xF2); //Use 0xF2 as TAG for special message
		UART_ECHO(0x9b);
		UART_ECHO('\n');
		return false;
	} else {
		return true;
	}
	//Loop back next time
}

void auto_approach_count(void)
{
	aappr.timer_count++;
}

void auto_approach_get_info(void){
	auto_approach_measure();
	auto_approach_send_info_payload();

}

void auto_approach_send_info(void){
	UART_ECHO('\n');
	UART_ECHO(0xF2);
	UART_ECHO(0x9C);
	auto_approach_send_info_payload();
	UART_ECHO('\n');
}

void auto_approach_send_info_payload(void){
	uart_write_char(aappr.state);
	uart_write_char((aappr.measurement));
	uart_write_char(((aappr.measurement >> 8)));
}

void auto_approach_measure(void){
	adc_start_conv(adc5);
	aappr.measurement = adc_get_val(); //adc_get_val() apparantely has race condition... to fix
}


/**
The general 1-ms timer, timer1, will call this function every ms.
To introduce a delay, simply count up on the aappr.timer_count variable until
desired ms is reached. 
*/

void auto_approach_state_machine(){
	switch(aappr.state) {
    case DISABLED: //Disabled state or Aborted state... safe shutdown
			stpr_exit_cont();
      stpr_sleep();
			aappr.state = DISABLED;
			auto_approach_send_info();
			//Do nothing
      break;
    case WAKEUP: //Wake up and intialization.
				//Clear the vars
				aappr.measurement = 0;
				aappr.measurement_init = 0;
				aappr.timer_count = 0;
        //Turn OFF PID
				pid_enable(false);
				auto_approach_measure(); //Measure value right now so that the first few auto_approach_send_info() has real ADCZ data
        //Stop and sleep the motor
        //stpr_exit_cont();
				//stpr_sleep();
        //Now wake it up
				stpr_wake();
				stpr_set_dir('f'); //Forward is down...
				stpr_set_speed(0x66BC);	//Very fast
				stpr_set_step(STEP_FULL);
		    stpr_cont(); //Back-up a bit
				auto_approach_send_info();
        aappr.state++;
				//callback required here
				aappr.timer_count = 0;
        break;
    case FAST_RETR_GO: 
				//200 millseconds of moving down...
				if(aappr.timer_count<200){
					//do nothing
				} else {
					auto_approach_send_info();
					aappr.state++;
				}
        break;
    case FAST_RETR_STOP: //Get initial measurement
				//Stop, switch direction, slow down a bit
        stpr_exit_cont();
				stpr_set_dir('b'); 
				stpr_set_speed(0x647D);	//A bit slower..
        //Measure Signal... ADC_ZOFFSET
        //Clear existing first
        aappr.measurement_init = 0;
        //Read the ADC #5
				auto_approach_measure();
				aappr.measurement_init = aappr.measurement; 
				auto_approach_send_info();
        aappr.state++;
				aappr.timer_count = 0;
				break;
		case INIT_MEAS: //Hold for 600msec, then measure
				if(aappr.timer_count<600){
					//do nothing
				} else {
					auto_approach_send_info();
					aappr.state++;
				}
        break;
    case PRE_FAST_APPR: //Prepare for High-speed operation
				//Get data from ADC
				auto_approach_measure();
        // The callback for readADC ADCZOFFSET should update autoappr_measurement
				if(aappr.measurement <= aappr.setpoint) {
						//Make sure that setpoint is OK.
						aappr.state = SETPOINT_REACHED;
						break;
				} else {
						// VERY IMPORTANT We also double check that the target is less than 95% of the init to begin with.
						// If it is more than 94%, then we should skip straight to state 6.
						if(aappr.setpoint <= (0.94*aappr.measurement_init)) {
								stpr_cont();
								auto_approach_send_info();
								aappr.state++;
								aappr.timer_count = 0; 
							//Do not reset the timer_count after here
							//The periodic call of auto_approach_send_info needs a clean start of timer_count to work
						} else {
								aappr.state = PRE_SLOW_APPR;
						}
				}
        break;
    case FAST_APPR: //High speed up, Abort available here.
        // Get measured signal. 
				auto_approach_measure();
        // If measured signal is less than whatever, do whatever
        // if not yet at .940 init, etc...
        // Actual logic
        // Now check measurement against target value.
        if(aappr.measurement <= (0.94*aappr.measurement_init)) {
						auto_approach_send_info();
            aappr.state++;
        } 
        break;
    case PRE_SLOW_APPR: //Reduce speed
				stpr_set_step(STEP_8);
				stpr_set_speed(0x4e20); //equivalent to 20000
				adc_start_conv(adc5);
				aappr.measurement = adc_get_val();
        stpr_cont();
				auto_approach_send_info();
        aappr.state++;
    case SLOW_APPR://Abort available here.
        //Loop like state #5
				auto_approach_measure();
        // Check if measurement is at setpoint autoappr_setpoint
        if(aappr.measurement <= aappr.setpoint) {
            stpr_exit_cont();
						auto_approach_send_info();
            aappr.state++;
        } 
        break;
    case SETPOINT_REACHED: 
        stpr_exit_cont();
        stpr_sleep();
        //Turn on PID
        pid_enable(true);
        //Clean Up
				auto_approach_send_info();
        aappr.state = DISABLED;
				auto_approach_send_info(); //Send a final DISABLED state message
        break;
	}
}

