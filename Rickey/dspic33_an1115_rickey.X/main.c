/***********************************************************************
 *                                                                     * 
 *    Author:         Darren Wenn                                      *
 *    Company:        Microchip Ltd                                    * 
 *    Filename:       main.c                                           *
 *    Date:           01/11/2006                                       *
 *    File Version:   7.00                                             *
 *    Other Files Required: project files			                   *
 *                                                                     *
 ***********************************************************************
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
* 
* V4.0	D.Wenn		Original version 
* V4.1	D.Wenn		Modified to support compile switch allowing
*					insertion of debugging code (#define in common.h)
* V5.0  D.Wenn		Reworked DSP to be more efficient
*					Software now uses continuous sampling method
* V6.0	D.Wenn		Further mods to assembly code and changes to filters,
* V7.0	D.Wenn		Cleanup 
*******************************************************************/
/// \file main.c
/// main program file

#include <p33Fxxxx.h>
#include "common.h"
#include <dsp.h>
#include <stdio.h>
#include "lcd.h"
#include "delay.h"
#include "string.h"

// \cond
_FGS(GWRP_OFF & GSS_OFF);
_FOSCSEL(FNOSC_PRIPLL);
_FOSC(FCKSM_CSDCMD & OSCIOFNC_OFF & POSCMD_XT);
_FWDT(FWDTEN_OFF);
// \endcond

/// Version string
const char* Version = "\rHAM GIRL\r"; 

// Frequency Sweep Globals
unsigned long t2;
unsigned int sweep_in_progress;
unsigned int t4_ms_counter;

/// Buffer located in DPRAM to hold the incoming ADC data.
/// This is 2x the requested size since ping-pong buffer mode is used
unsigned int ADCData[DMA_BUFFER_SIZE * 2] __attribute__((space(dma)));

/// Flag indicating valid data has just been placed in the fIn & fQn variables.
volatile unsigned short SampleReady;

/// \enum stateDisplay enumerated type for controlling display mode.
enum stateDisplay {
	DISPLAY_DEFAULT,
	DISPLAY_SINGLE,
	DISPLAY_CONT,
} stateDisplay;

///////////////////////////////////////////////////////////////////
///
/// firInit set up the FIR filter parameters.
///
///////////////////////////////////////////////////////////////////
void filterInit(void)
{
	// initialise the FIRstructure
	FIRDelayInit(&fir1500FilterI);
	FIRDelayInit(&fir1500FilterQ);
	FIRDelayInit(&fir5300FilterI);
	FIRDelayInit(&fir5300FilterQ);
	FIRDelayInit(&fir150FilterI);
	FIRDelayInit(&fir150FilterQ);
	FIRDelayInit(&fir0012FilterI);
	FIRDelayInit(&fir0012FilterQ);
}

///////////////////////////////////////////////////////////////////
///
/// The function to change frequency of sine wave generated
///
///////////////////////////////////////////////////////////////////

void changeFreq() {
    unsigned int i1, i2, i3;
    unsigned int outVal;
    long accumulatedDC;
    
    i2 = ((unsigned long)(2500000/t2)-1);
    i3 = (unsigned long)(2500000/t2)*4-1;
 
    T2CONbits.TON = 0;
    PR2 = ((unsigned long)(2500000/t2)-1);
    TMR2 = 0;
    T3CONbits.TON = 0;
    PR3 = (unsigned long)(2500000/t2)*4-1;
    TMR3 = 0;
    IFS0bits.T3IF = 0; 
    
    //Init_ADC();

    //Turn both on as closely as possible
    TMR2 = 0;
    TMR3 = 0;
    T2CONbits.TON = 1;
    T3CONbits.TON = 1;
    
    return;
}

///////////////////////////////////////////////////////////////////
///
/// The main function.
///
///////////////////////////////////////////////////////////////////
int main ( void )
{
	unsigned int i, j;		// temporary index
	unsigned int outVal;
	long accumulatedDC;
	fractional tI, tQ;
	float fI, fQ;
	float mag, phi;
	char sBuff[40];
	
    //The settings below set up the oscillator and PLL for 40 MIPS
    // set feedback divisor bits for multiply by 40 (38 + 2) -> 160MHz / 2 = 80MHz
    PLLFBD = 0x0026;
    // set the clock divide register for PLLPRE = 0 (divide by 2)
    // and the PLLPOST = 2 (divide by 2)
    CLKDIV = 0x0000;
    
 	LATA = 0xFF00; 
	TRISA = 0xFF00; 
	LATB = 0xFFF0;
 	TRISB = 0xFFF0;
 	TRISC = 0x0000;
 	LATD = 0xF800;
 	TRISD = 0xFFFF;
 	TRISFbits.TRISF5 = 0;	// uart TX output pin
	TRISG &= 0x0FFF;

	Delay(Delay_15mS_Cnt);

	// set up the UART
	InitRS232();
	Delay(Delay_15mS_Cnt);	
	RS232XMT((char*) Version);
	
	/* Initialize LCD Display */
	Init_LCD();
	/* Welcome message */
	home_clr();
	Delay(Delay_15mS_Cnt);	
	puts_lcd( (unsigned char*) Version, strlen(Version) -1 );
	Delay(Delay_15mS_Cnt);	
	
	stateDisplay = DISPLAY_DEFAULT;
    filterInit();
    Init_ADC();
    Init_Timers();
	
	// perform an initial sample set to get the DC offset without the
	// T2 ISR running
	outVal = LATG & 0x0FFF;
	LATG = (63 << 10) | outVal;
	SampleOffset = 0;
	TMR3 = 0;
	T3CONbits.TON = 1;
	Delay(Delay_15mS_Cnt);
	DCOffset = 0;
	SampleOffset = 1;
	accumulatedDC = 0;
	for(i = 0; i < 16; i++) {
		SampleReady = 0;
		while (SampleReady == 0);
		accumulatedDC += DCCurrentReading;
	}
	SampleOffset = 0;
	T3CONbits.TON = 0;
	DCOffset = (fractional) (accumulatedDC / 16);
	DCOffset /= 2;	// because we are outputting the full signal
	DCOffset = -DCOffset;
	
	// turn on the conversion timer to trigger the DMAs
	// by setting TMR5 = xxx (between 0 and 399) you can adjust 
	// the phase relationship in the 0-90 range from its basic offset
	TMR2 = 0;
	TMR3 = 0;
	T2CONbits.TON = 1;
	T3CONbits.TON = 1;
    
    //Sweep related features
    T4CONbits.TON = 1;
    sweep_in_progress = 0;
    t4_ms_counter = 0;
	
    while (1) {
         // Change frequency when BUTTON4 is pressed
        if (BUTTON4 == 0) {
            // Prepare for sweep
            // Measure DC offset
            // perform an initial sample set to get the DC offset without the
            // T2 ISR running
            T2CONbits.TON = 0;
            outVal = LATG & 0x0FFF;
            LATG = (63 << 10) | outVal;
            SampleOffset = 0;
            TMR3 = 0;
            T3CONbits.TON = 1;
            Delay(Delay_15mS_Cnt);
            DCOffset = 0;
            SampleOffset = 1;
            accumulatedDC = 0;
            for(i = 0; i < 16; i++) {
                SampleReady = 0;
                while (SampleReady == 0);
                accumulatedDC += DCCurrentReading;
            }
            SampleOffset = 0;
            T3CONbits.TON = 0;
            DCOffset = (fractional) (accumulatedDC / 16);
            DCOffset /= 2;	// because we are outputting the full signal
            DCOffset = -DCOffset;   
            
            // Start sweeping
            sweep_in_progress = 1;
            t2 = 7500;
            T2CONbits.TON = 1;
            T3CONbits.TON = 1;
            
            while (BUTTON4 == 0);
            changeFreq();
//            if(t2==5000){
//                t2 = 6660;
//            } else if (t2==6660){
//                t2 = 8000;
//            } else {
//                t2 = 5000;
//            }
            //t2 = t2 + 1000;
        }
        if (sweep_in_progress == 1 && t4_ms_counter > 1000) {
            t4_ms_counter = 0;
            if (t2 > 8150) {
                sweep_in_progress = 0;
            } else {
                changeFreq();
                t2 = t2 + 5;
            }
        }
        
	    // wait for the next block of processed data SampleReady set in DMA ISR
	    // synchronise with DMA routine to ensure clean data
		SampleReady = 0;
		while (SampleReady == 0);
		
		// handle sync change
//		if (BUTTON4 == 0) {
//			TMR5++;
//			while (BUTTON4 == 0);	
//		}
		
		// copy sample values across to local storage so they do not
		// get corrupted by the ISRs if we take a while processing them here	
		tI = fin;
		tQ = fqn;
		
		switch (stateDisplay) {
			case DISPLAY_DEFAULT:
				if (BUTTON1 == 0) 
					stateDisplay = DISPLAY_SINGLE;
				if (BUTTON2 == 0)
					stateDisplay = DISPLAY_CONT;
				break;
			case DISPLAY_SINGLE:
				fI = Fract2Float(tI);
				fQ = Fract2Float(tQ);
		
				mag = sqrt(fI * fI + fQ * fQ);
				phi = atan2(fQ, fI) * 180.0f / PI;
				sprintf(sBuff,"%8.4f, %8.4f\r", mag, phi);
				RS232XMT(sBuff);
				stateDisplay = DISPLAY_DEFAULT;
				break;
			case DISPLAY_CONT:	
				fI = Fract2Float(tI);
				fQ = Fract2Float(tQ);
				mag = sqrt(fI * fI + fQ * fQ);
				phi = atan2(fQ, fI) * 180.0f / PI;
				
#ifdef USE_LCD
			    home_it();	 
	    		sprintf(sBuff, "Mag = %8.5f     ", mag);
				puts_lcd(sBuff, strlen(sBuff));	    
			    line_2();
			    sprintf(sBuff, "Phi = %8.3f     ", phi);
			    puts_lcd(sBuff, strlen(sBuff));
#endif
			    
				sprintf(sBuff, "%8.4f, %8.4f, %lu\r", fI, fQ, t2);
				RS232XMT(sBuff);
				if (BUTTON1 == 0)
					stateDisplay = DISPLAY_DEFAULT;
				break;				
		}
	}
}


