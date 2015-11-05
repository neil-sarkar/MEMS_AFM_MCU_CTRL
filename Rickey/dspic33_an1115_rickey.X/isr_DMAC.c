/**********************************************************************
* © 2006 Microchip Technology Inc.
*
* FileName:        isr_DMAC.c
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
* Darren Wenn		13/02/06  Created to handle ADC data
*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
* ADDITIONAL NOTES:
*
*
**********************************************************************/
/// \file isr_DMAC.c
/// DMA interrupt service routine function and variables

#include <p33Fxxxx.h>
#include "common.h"
#include <dsp.h>

///	Working buffers for the Inphase and Quadrature signals.
///	Half the incoming data size because of the
///	alternating zero entries which are ignored.
volatile fractional In[DMA_BUFFER_SIZE / 2];
/// Quadrature signal
volatile fractional Qn[DMA_BUFFER_SIZE / 2];

/// fractional output value from the current calculation.
volatile fractional fin;
/// quadrature output value
volatile fractional fqn;

/// arrays to hold the intermediate decimated data.
volatile fractional fTI[DMA_BUFFER_SIZE / 20];
/// quadrature decimated data
volatile fractional fTQ[DMA_BUFFER_SIZE / 20];

volatile unsigned short SampleOffset;
volatile fractional DCOffset;
volatile fractional DCCurrentReading;

///////////////////////////////////////////////////////////////////
///
/// _DMA0Interrupt
///
/// ISR handler routine for DMA channel 0
/// Receive incoming ADC data, perform the quadrature mixing
///	and then low pass filter the result to give the final outputs
///////////////////////////////////////////////////////////////////
void __attribute__((__interrupt__,no_auto_psv)) _DMA0Interrupt( void ) 
{
	fractional* ptCopySig;		
	unsigned int i;
	long avValue;
	
	// copy the data to the result buffer
	if (DMACS1bits.PPST0 == 0) {
		// data is in the first part of the ADC buffer	
		ptCopySig = (fractional *)&ADCData[0];
	} else {
		// data is in the second part of the ADC buffer
		ptCopySig = (fractional *)&ADCData[DMA_BUFFER_SIZE];
	}	

	// sample the DC offset values and average them
	if (SampleOffset == 1) {
		avValue = 0;
		for(i = 0; i < DMA_BUFFER_SIZE; i++) {
			avValue += *ptCopySig++;
		}
		DCCurrentReading = (fractional) (avValue / DMA_BUFFER_SIZE);
		SampleReady = 1;
		IFS0bits.DMA0IF = 0;
		return;
	}
	
	// process the sample block returning DMA_BUFFER_SIZE/2 sample pairs
	quadratureMult(DMA_BUFFER_SIZE, ptCopySig, 
		(fractional*) In, (fractional*) Qn, DCOffset);
		
	// low pass filter the data and decimate by 10 giving an
	// effective sampling rate of 5kHz
	for(i = 0; i < (DMA_BUFFER_SIZE / 20); i++) {
		fTI[i] = foldedFIR(10, (fractional*) &In[i * 10],
			&fir1500FilterI);
		fTQ[i] = foldedFIR(10, (fractional*) &Qn[i * 10],
			&fir1500FilterQ);		
	}

	// low pass and decimate by 8 giving sample rate of 625Hz
	// and 2 output samples per channel
	fTI[0] = foldedFIR(8, (fractional*) &fTI[0], &fir5300FilterI);
	fTI[1] = foldedFIR(8, (fractional*) &fTI[8], &fir5300FilterI);
	fTQ[0] = foldedFIR(8, (fractional*) &fTQ[0], &fir5300FilterQ);
	fTQ[1] = foldedFIR(8, (fractional*) &fTQ[8], &fir5300FilterQ);
	
	// decimate once more to give an effective sample rate of 312.5Hz and 1 sample
	fTI[0] = foldedFIR(2, (fractional*) &fTI[0], &fir150FilterI);
	fTQ[0] = foldedFIR(2, (fractional*) &fTQ[0], &fir150FilterQ);

	// very low frequency final filter with Fpass=0.001Hz and Fstop=2Hz
	FIR(1, (fractional*) fTI, (fractional*) &fTI[0], &fir0012FilterI);
	FIR(1, (fractional*) fTQ, (fractional*) &fTQ[0], &fir0012FilterQ);
	fin = fTI[0];
	fqn = fTQ[0];
	
	SampleReady = 1;

	IFS0bits.DMA0IF = 0;            // reset DMA interrupt flag  
}
