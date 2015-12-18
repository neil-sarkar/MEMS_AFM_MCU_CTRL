/**********************************************************************
* © 2006 Microchip Technology Inc.
*
* FileName:        init_timer1.c
* Dependencies:    p33FJ256GP710.h
* Processor:       dsPIC33F
* Compiler:        MPLAB® C30 v2.01 or higher
*
* SOFTWARE LICENSE AGREEMENT:
* Microchip Technology Incorporated ("Microchip") retains all ownership and 
* intellectual property rights in the code accompanying this message and in all 
* derivatives hereto.  You may use this code, and any derivatives created by 
* any person or entity by or on your behalf, exclusively with Microchip's
* proprietary products.  Your acceptance and/or use of this code constitutes 
* agreement to the terms and conditions of this notice.
*
* CODE ACCOMPANYING THIS MESSAGE IS SUPPLIED BY MICROCHIP "AS IS".  NO 
* WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED 
* TO, IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A 
* PARTICULAR PURPOSE APPLY TO THIS CODE, ITS INTERACTION WITH MICROCHIP'S 
* PRODUCTS, COMBINATION WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION. 
*
* YOU ACKNOWLEDGE AND AGREE THAT, IN NO EVENT, SHALL MICROCHIP BE LIABLE, WHETHER 
* IN CONTRACT, WARRANTY, TORT (INCLUDING NEGLIGENCE OR BREACH OF STATUTORY DUTY), 
* STRICT LIABILITY, INDEMNITY, CONTRIBUTION, OR OTHERWISE, FOR ANY INDIRECT, SPECIAL, 
* PUNITIVE, EXEMPLARY, INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, FOR COST OR EXPENSE OF 
* ANY KIND WHATSOEVER RELATED TO THE CODE, HOWSOEVER CAUSED, EVEN IF MICROCHIP HAS BEEN 
* ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT 
* ALLOWABLE BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY RELATED TO 
* THIS CODE, SHALL NOT EXCEED THE PRICE YOU PAID DIRECTLY TO MICROCHIP SPECIFICALLY TO 
* HAVE THIS CODE DEVELOPED.
*
* You agree that you are solely responsible for testing the code and 
* determining its suitability.  Microchip has no obligation to modify, test, 
* certify, or support the code.
*
* REVISION HISTORY:
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* Author            Date      Comments on this revision
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
* Darren Wenn	  15/02/06  Set for Timer1 interrupt @ 25Hz
*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
* ADDITIONAL NOTES:
*
*
**********************************************************************/
/// \file init_timers.c

#include <p33Fxxxx.h>
#include "common.h"
#include <float.h>

// Frequency Sweep Globals
extern unsigned long t2;

///////////////////////////////////////////////////////////////////
///
/// Init_Timers
///
/// Initialise the timers such that TMR 2 will interrupt at 400kHz
/// and will be used to generate the 25kHz output waveform. TMR 3 is
/// set to generate a 100kHz interrupt used for the ADC start conversion
///////////////////////////////////////////////////////////////////
void Init_Timers( void )
{
    t2 = 5000;
            
	// set up timer 2 to interrupt at at a rate of 16 points per full
	// wave at 25kHz (this equals an interrupt rate of 400kHz)
	T2CON = 0;
	IFS0bits.T2IF = 0;
	IPC1bits.T2IP = 5;
	PR2 = ((unsigned long)(2500000/t2)-1); //N.B. this is a simplified expression
	TMR2 = 0;
	IEC0bits.T2IE = 1;
	
	// set up TMR3 to generate signals for the ADC convert
	// every 400 cycles or 100kHz
	T3CON = 0;
	PR3 = (unsigned long)(2500000/t2)*4-1; //N.B. this is a simplified expression
	TMR3 = 0;
	IFS0bits.T3IF = 0;
	
	// delay turning the output timer on until the main loop
	T2CONbits.TON = 0;
    
    // Set up TMR4 for interrupting our ms counter
    T4CON = 0;
    T4CONbits.TCKPS = 0x0;  //Prescaler to 0
	PR4 = 40000-1; //One tick each 1ms
	TMR4 = 0;
    IFS1bits.T4IF = 0; // Clear Timer4 Interrupt Flag
    IEC1bits.T4IE = 1; // Enable Timer4 interrupt
}
