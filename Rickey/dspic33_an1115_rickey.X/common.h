
/**********************************************************************
* © 2006 Microchip Technology Inc.
*
* FileName:        common.h
* Dependencies:    none
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
* Darren Wenn      01/01/06  General project configuration
*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
* ADDITIONAL NOTES:
* 1. This file contains definitions commonly used in this project.
*
**********************************************************************/

#include "dsp.h"

// define this to enable output on the LCD
#define USE_LCD 1

// number of elements in each DMA buffer
#define DMA_BUFFER_SIZE 320

// define the buttons on the board
#define BUTTON1 PORTDbits.RD6
#define BUTTON2 PORTDbits.RD7
#define BUTTON3 PORTAbits.RA7
#define BUTTON4 PORTDbits.RD13

// this buffer receives the ADC samples as they come in
// it is allocated in dma memory so that the DMAC can 
// automate the sampling and transfer
extern unsigned int __attribute__((space(dma))) ADCData[DMA_BUFFER_SIZE * 2];
extern void Init_Timers(void);
extern void InitRS232(void);
extern void RS232XMT(char* ptx);

extern volatile unsigned short SampleOffset;
extern volatile fractional DCOffset;
extern volatile fractional DCCurrentReading;

extern volatile unsigned short SampleReady;
extern volatile unsigned int SampleCount;

// fractional I and Q values
extern volatile fractional fin;
extern volatile fractional fqn;

// routines in foldedFIR.s
void quadratureMult(unsigned int num, fractional* src, 
	fractional* i, fractional* q, fractional DCOffset);
fractional foldedFIR(unsigned int num, fractional* src, FIRStruct* fir);

///////////////////////////////////////////////////////////////////
//
// Data for the filters

extern FIRStruct fir1500FilterI;
extern FIRStruct fir1500FilterQ;

extern FIRStruct fir5300FilterI;
extern FIRStruct fir5300FilterQ;

extern FIRStruct fir150FilterI;
extern FIRStruct fir150FilterQ;

extern FIRStruct fir0012FilterI;
extern FIRStruct fir0012FilterQ;


