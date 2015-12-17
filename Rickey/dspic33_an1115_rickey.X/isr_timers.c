/**********************************************************************
* © 2006 Microchip Technology Inc.
*
* FileName:        isr_timer1.c
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
* Darren Wenn		15/02/06  Core ISR for sinewave generation
*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
* ADDITIONAL NOTES:
*
*
**********************************************************************/
/// \file isr_timers.c

#include <p33Fxxxx.h>
#include "common.h"

extern unsigned int sweep_in_progress;
extern unsigned int t4_ms_counter;

/// 16 point table describing the reference signal 
/// with 6 bit resolution although only 4 bits are actually used
static unsigned char sinTable[] = {
	32, 44, 54, 61,
	63, 60, 54, 44, 
	32, 20, 10,  3,
	 1,  3, 10, 20
};

/// static variables to reduce stack frame setup in ISR
volatile unsigned char _sinTableIndex;
volatile unsigned int _outVal;

///////////////////////////////////////////////////////////////////
///
/// _T2Interrupt
///
/// Timer 2 ISR handler. Iterate through the table containing the
/// sinusoidal output waveform and apply it to the output port.
///////////////////////////////////////////////////////////////////
void __attribute__((__interrupt__,no_auto_psv)) _T2Interrupt( void )
{
	// this code generates a stepped sinusoidal output
	// on pins LATD<8:13> and on G<15:12>
	// since this is phase locked with the ADC sampling clock
	// this routine must not be interrupted
	_sinTableIndex++;
	_sinTableIndex &= 0b00001111;

	_outVal = LATG & 0x0FFF;
	LATG = (sinTable[_sinTableIndex] << 10) | _outVal;

	IFS0bits.T2IF = 0;
}

///////////////////////////////////////////////////////////////////
///
/// _T4Interrupt
///
/// Timer 4 is a one-ms interrupt. It interrupts every ms and updates the values
// http://www.microchip.com/forums/m874835.aspx
///////////////////////////////////////////////////////////////////
void __attribute__((__interrupt__,no_auto_psv)) _T4Interrupt( void )
{
	/* Interrupt Service Routine code goes here */
    t4_ms_counter++;
    IFS1bits.T4IF = 0; // Clear Timer4 Interrupt Flag
}
