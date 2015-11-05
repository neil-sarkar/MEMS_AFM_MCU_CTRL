/**********************************************************************
* © 2006 Microchip Technology Inc.
*
* FileName:        RS232.c
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
* Darren Wenn		06/10/06  Initial Version
*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
* ADDITIONAL NOTES:
*
*
**********************************************************************/
/// \file rs232.c

#include <p33fxxxx.h>
#include "common.h"

/// Receive data buffer
char RS232Buff[80];
/// Pointer to receive buffer
char* ReceiveData = RS232Buff;
/// Transmit buffer
char TxBuff[40];

///////////////////////////////////////////////////////////////////
/// _U2TXInterrupt.
/// UART2 Transmit interrupt
///////////////////////////////////////////////////////////////////
void __attribute__((__interrupt__,no_auto_psv)) _U2TXInterrupt(void)
{
	IFS1bits.U2TXIF = 0;
}

///////////////////////////////////////////////////////////////////
/// _U2RXInterrupt.
/// UART2 Receive interrupt. Store data in the input buffer and
/// update the receive data pointer.
///////////////////////////////////////////////////////////////////
void __attribute__((__interrupt__,no_auto_psv)) _U2RXInterrupt(void)
{
	IFS1bits.U2RXIF = 0;
	while (U2STAbits.URXDA) 
	{
		(*(ReceiveData)++) = U2RXREG;
	}	
}

///////////////////////////////////////////////////////////////////
/// InitRS232.
/// Initialise the UART for 115200 Baud, 8N1
///////////////////////////////////////////////////////////////////
void InitRS232(void)
{
	U2MODEbits.UARTEN = 0;
	U2STAbits.UTXISEL1 = 1;
	U2STAbits.UTXISEL0 = 0;
	U2STAbits.URXISEL = 0;
	U2STAbits.OERR = 0;
	U2BRG = 21;
	IFS1bits.U2RXIF = 0;
	IEC1bits.U2RXIE = 1;
	U2MODEbits.UARTEN = 1;
	U2STAbits.UTXEN = 1;
}

///////////////////////////////////////////////////////////////////
/// RS232XMT.
/// Transmit a NULL terminated data string from UART2
/// @param ptx NULL terminated data.
///////////////////////////////////////////////////////////////////
void RS232XMT(char* ptx)
{
	while (*ptx != '\0') {
		if (U2STAbits.UTXBF == 0) {
			U2TXREG = *ptx;
			ptx++;
		}
	}
}




