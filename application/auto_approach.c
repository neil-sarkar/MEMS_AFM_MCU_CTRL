#include "auto_approach.h"

// Default number of ticks for approach timer 
#define TMR_DFLT  5000

/*
	A generic, motor-independent, auto approach alogirthm 
*/

struct auto_approach 
{
	u8 state;
	u16 setpoint;
	u16 measurement_init;
	u16 measurement;
	

/* Auto Approach Feature */
QTimer *task1_timer;   //For auto approach

bool autoapproach_abort = false;



int autoapproach_fault_count = 0;
};

struct auto_approach aappr;

void auto_approach_init (void)
{
	/* Initilize Timer 2 for the coarse approach */
	T2LD  = TMR_DFLT;
	T2CON = BIT6 + BIT9;						// Periodic mode, core clock

}

void auto_approach_start(void)
{
		/*
	u8 val_l, val_h;

	val_l = uart_wait_get_char();
	val_h = uart_wait_get_char();
		*/
	//Init Timers
	
	
	aappr.state = 1;
	uart_wait_get_bytes((u8*)(&aappr.setpoint), 2);
							// Enable Timer2 fast interrupt
}

void auto_approach_get_info(void){
	
	
}

void auto_approach_state_machine(){
	switch(aappr.state) {
    case 0: //Disabled state
				// Stop timer
				stpr_exit_cont();
				stpr_sleep();
        break;
    case 1: //Wake up and intialization.
        //Turn OFF PID
				pid_enable(false);
        //Stop and sleep the motor
        stpr_exit_cont();
				stpr_sleep();
        //Now wake it up
				stpr_wake();
				stpr_set_dir('f'); //Forward is down...
				stpr_set_speed(0xbc66);	//Very fast
				stpr_set_step(0x01);
        aappr.state++;
				//callback required here
        break;
    case 2: //Back-up a bit
        stpr_cont();
				
        aappr.state++;
				//200 millseconds later...
        QTimer::singleShot(200, this, SLOT(autoApproach_state_machine()));
        break;
    case 3: //Get initial measurement
        stpr_exit_cont();
        //Measure Signal... ADC_ZOFFSET
        //Clear existing first
        aappr.measurement_init = -1;
        //Read the ADC #5
				// Read ADC
				adc_start_conv(0x05);
				aappr.measurement_init = adc_get_val();
        aappr.state++;
        QTimer::singleShot(600, this, SLOT(autoApproach_state_machine()));
        break;
    case 4: //Continuous ON
				//Get data from ADC
				adc_start_conv(0x05);
				aappr.measurement = adc_get_val(); //adc_get_val() apparantely has race condition... maybe we shouldn't have it here?
		//Like, it could die...
        // The callback for readADC ADCZOFFSET should update autoappr_measurement
            if(aappr.measurement <= aappr.setpoint) {
                //Make sure that setpoint is OK.
                aappr.state = 8;
                QTimer::singleShot(1, this, SLOT(autoApproach_state_machine()));
                break;
            } else {
                //if signal received then save measurement_init and proceed.
                // VERY IMPORTANT We also double check that the target is less than 95% of the init to begin with.
                // If it is more than 95%, then we should skip straight to state 6.
                if(aappr.setpoint <= (0.95*aappr.measurement_init)) {
                    commandQueue.push(new commandNode(stepMotContGo));
                    autoapproach_state++;
                    task1_timer->start(20);
                } else {
                    autoapproach_state = 6;
                    QTimer::singleShot(1, this, SLOT(autoApproach_state_machine()));
                }
            }
        break;
    case 5: //Abort available here.
        // Get measured signal. If measured signal expected has not been received, then uhhh
        // If measured signal is less than whatever, do whatever
        // if not yet at .95 init, etc...)

        // Actual logic
        // First check measurement validity
        if (aappr.measurement == -1 && autoapproach_fault_count < MAX_AUTOAPPR_FAULT_COUNT) {
            qDebug() << "autoappr_measurement has not been updated yet!";
            autoapproach_fault_count++;
        } else if (autoapproach_fault_count >= MAX_AUTOAPPR_FAULT_COUNT) {
            autoapproach_state = 0;
            qDebug() << "AutoAppr automatic abort. autoappr_measurement=" << autoappr_measurement << " autoapproach_fault_count="<<autoapproach_fault_count;
            break;
        } else {
            autoapproach_fault_count = 0;
        }
        // Now check measurement against target value.
        if((autoappr_measurement <= (0.95*autoappr_measurement_init)) && autoappr_measurement > 0) {
            task1_timer->stop();
            autoapproach_state++;
            QTimer::singleShot(1, this, SLOT(autoApproach_state_machine()));
        } else if (autoappr_measurement != -1) {
            //Continue running the motor
            //Measure Signal... from ADC_ZOFFSET
            //task1_timer will keep calling the state machine
            commandQueue.push(new commandNode(readADC, (qint8)ADC_ZOFFSET));
        }
        // Reset measurement var
        autoappr_measurement = -1;
        break;
    case 6: //Reduce speed
        ui->progbar_autoappr->setValue(6);
        commandQueue.push(new commandNode(stepMotSetMicrostep, (qint8)3));
        commandQueue.push(new commandNode(stepMotSetSpeed, (double)20000));
        commandQueue.push(new commandNode(readADC, (qint8)ADC_ZOFFSET));
        commandQueue.push(new commandNode(stepMotContGo));
        autoapproach_state++;
        task1_timer->start(20);
    case 7://Abort available here.
        ui->progbar_autoappr->setValue(7);
        // Update display
        if(autoappr_measurement > 0) {
            QString s = QString::number(autoappr_measurement);
            ui->label_autoappr_meas->setText(s);
        }
        //Loop like state #5
        // First check measurement validity
        if (autoappr_measurement == -1 && autoapproach_fault_count < MAX_AUTOAPPR_FAULT_COUNT) {
            qDebug() << "autoappr_measurement has not been updated yet!";
            autoapproach_fault_count++;
        } else if (autoapproach_fault_count >= MAX_AUTOAPPR_FAULT_COUNT) {
            autoapproach_state = 0;
            qDebug() << "AutoAppr automatic abort. autoappr_measurement=" << autoappr_measurement << " autoapproach_fault_count="<<autoapproach_fault_count;
            break;
        } else {
            autoapproach_fault_count = 0;
        }
        // Check if measurement is at setpoint autoappr_setpoint
        if((autoappr_measurement <= autoappr_setpoint) && autoappr_measurement > 0) {
            task1_timer->stop();
            autoapproach_state++;
            QTimer::singleShot(1, this, SLOT(autoApproach_state_machine()));
        } else if (autoappr_measurement != -1) {
            //Continue running the motor
            //Measure Signal... from ADC_ZOFFSET
            //task1_timer will keep calling the state machine
            commandQueue.push(new commandNode(readADC, (qint8)ADC_ZOFFSET));
        }
        autoappr_measurement = -1;
        break;
    case 8:
        ui->progbar_autoappr->setValue(8);
        commandQueue.push(new commandNode(stepMotContStop));
        commandQueue.push(new commandNode(stepMotSetState, qint8(MOT_SLEEP)));
        //UPDATE UI
        //Turn on PID
        commandQueue.push(new commandNode(pidEnable));
        //Clean Up
        autoapproach_state = 0;
        QTimer::singleShot(1, this, SLOT(autoApproach_state_machine()));
        break;
}




if (rx_fifo.rx == RESET_INITIATOR_CHAR && rst_state == NO_RESET) 	
															{ 																																
																rst_state = LAYER1; 																						
															}																																	
															else if (rst_state != NO_RESET)																		
															{																																	
																switch (rst_state)																							
																{																																
																	case LAYER1:																									
																		if (rx_fifo.rx == LAYER1_RESET_CHAR)												
																			rst_state = LAYER2;																				
																		else																												
																			rst_state = NO_RESET;																			
																		break;																											
																	case LAYER2:																									
																		if (rx_fifo.rx == LAYER2_RESET_CHAR)												
																			rst_state = LAYER3;																				
																		else																												
																			rst_state = NO_RESET;																			
																		break;																											
																	case LAYER3:																									
																		if (rx_fifo.rx == LAYER3_RESET_CHAR)												
																			rst_state = LAYER4;																				
																		else																												
																			rst_state = NO_RESET;																			
																		break;																											
																	case LAYER4:																									
																		rst_state = NO_RESET;																				
																		if (rx_fifo.rx == LAYER4_RESET_CHAR)												
																			RSTSTA |= BIT2;																						
																		break;																											
																	default:																											
																		rst_state = NO_RESET;																				
																		break;																											
																}																																
															}																		