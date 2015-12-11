/**********************************************************************
* © 2006 Microchip Technology Inc.
*
* FileName:        init_ADC.c
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
* Darren Wenn		13/02/06  Created 
*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
* ADDITIONAL NOTES:
*
*
**********************************************************************/
/// \file init_ADC.c

#include <p33Fxxxx.h>
#include "common.h"
    
///////////////////////////////////////////////////////////////////
///
/// Init_ADC
///
/// Initialize the ADC module to 12-bit mode, sampling only channel 8
/// and conversion triggered by TMR 3. Output is to be in 16-bit signed
/// fractional format.
/// Also initialise the DMA controller Channel 0 to transfer DMA_BUFFER_SIZE
/// samples from the ADC to the DPRAM before generating an interrupt.
///////////////////////////////////////////////////////////////////
void Init_ADC( void )
{
	// set port configuration with AN8 going to ADC2 
	AD1PCFGLbits.PCFG8 = 0;
 	
 	// set no channel scanning, auto sampling and convert
 	// format is signed fractional l, enable 12 bit mode
 	AD1CON1bits.ADDMABM = 1;
 	AD1CON1bits.AD12B = 1; 
 	AD1CON1bits.FORM = 3;		// signed fractional format
 	AD1CON1bits.SSRC = 0b010;	// trigger conversion from TMR3
 	AD1CON1bits.ASAM = 1;
 	
	// set for no channel scanning, mux A, increment DMA by 1, Vref = AVdd/AVss
	AD1CON2 = 0x0000;

	// set samples and bit conversion time
	// use auto sample time of 1
	// note that conversion is triggered from TMR3
	AD1CON3bits.SAMC = 2;	// 
	AD1CON3bits.ADCS = 5;	// 	
	
	// select no channels to be scanned
	AD1CSSL = 0x0000;  
	AD1CHS0 = 0x0008;
	
	// allocate 1 word of buffer for each ADC channel
	AD1CON4bits.DMABL = 0b000;
	
	// clear the status register
	DMACS0 = 0;	
	// define the dma addresses to write to, note the start B
	// addresses since we use a ping pong mode
	DMA0STA = __builtin_dmaoffset(&ADCData[0]);
	DMA0STB = DMA0STA + DMA_BUFFER_SIZE * 2;	
	// define the peripheral addresses to read from
	DMA0PAD = (volatile unsigned int) &ADC1BUF0;
	// set up count of number of items to transfer (-1)
	DMA0CNT = DMA_BUFFER_SIZE - 1;	
	// set up control register, continuous ping-pong transfer,
	// register indirect mode, interrupt when all data transferred
	DMA0CON = 0x0002;
	// define the ADC irqs that the DMA controller should respond to
	DMA0REQ = 13;
	
	// enable DMA interrupts
	IFS0bits.DMA0IF = 0;
	IEC0bits.DMA0IE = 1;
	// set DMA interrupt priorities
	IPC1bits.DMA0IP = 4;
	
	// turn on ADC modules
	AD1CON1bits.ADON = 1;   
	
	// turn on DMA module
	DMA0CONbits.CHEN = 1; 
}
