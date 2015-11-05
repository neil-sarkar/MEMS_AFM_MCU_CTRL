/**********************************************************************
* © 2006 Microchip Technology Inc.
*
* FileName:        lcd.c
* Dependencies:    p33FJ256GP710.h
*                  lcd.h
*                  delay.h
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
* Richard Fischer   07/14/05  Explorer 16 board LCD support
* Priyabrata Sinha  01/27/06  Ported to non-prototype devices
*
*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
* ADDITIONAL NOTES:
*
**********************************************************************/
/// \file lcd.c

#include <p33Fxxxx.h>
#include "common.h"
#include "lcd.h"
#include "delay.h"
	
		
/* 
   For Explorer 16 board, here are the data and control signal definitions
   RS -> RB15
   E  -> RD4
   RW -> RD5
   DATA -> RE0 - RE7   
*/

// Control signal data pins 
#define  RW  LATDbits.LATD5       // LCD R/W signal
#define  RS  LATBbits.LATB15      // LCD RS signal
#define  E   LATDbits.LATD4       // LCD E signal 
//#define  E   LATFbits.LATF1       // LCD E signal

// Control signal pin direction 
#define  RW_TRIS	TRISDbits.TRISD5 
#define  RS_TRIS	TRISBbits.TRISB15
#define  E_TRIS		TRISDbits.TRISD4
//#define  E_TRIS		TRISFbits.TRISF1

// Data signals and pin direction
#define  DATA      LATE           // Port for LCD data
#define  DATAPORT  PORTE
#define  TRISDATA  TRISE          // I/O setup for data Port

///////////////////////////////////////////////////////////////////
/// Init_LCD.
/// Initialize the LCD to 8-bit output mode.
///////////////////////////////////////////////////////////////////
void Init_LCD( void )             // initialize LCD display
{
	// 15mS delay after Vdd reaches nnVdc before proceeding with LCD initialization
	// not always required and is based on system Vdd rise rate
	Delay(Delay_15mS_Cnt);                  // 15ms delay
			
	/* set initial states for the data and control pins */
	LATE &= 0xFF00;	
    RW = 0;                       // R/W state set low
	RS = 0;                       // RS state set low
	E = 0;                        // E state set low

	/* set data and control pins to outputs */
	TRISE &= 0xFF00;
 	RW_TRIS = 0;                  // RW pin set as output
	RS_TRIS = 0;                  // RS pin set as output
	E_TRIS = 0;                   // E pin set as output

	/* 1st LCD initialization sequence */
	DATA &= 0xFF00;
    DATA |= 0x0038;
    E = 1;	
    Nop();
    Nop();
    Nop();
    E = 0;                        // toggle E signal
   	Delay(Delay_5mS_Cnt);         // 5ms delay
      
	/* 2nd LCD initialization sequence */
	DATA &= 0xFF00;
    DATA |= 0x0038;
    E = 1;	
    Nop();
    Nop();
    Nop();	
    E = 0;                        // toggle E signal
    Delay_Us( Delay200uS_count ); // 200uS delay

	/* 3rd LCD initialization sequence */
	DATA &= 0xFF00;
    DATA |= 0x0038;
    E = 1;		
    Nop();
    Nop();
    Nop();	
    E = 0;                        // toggle E signal
    Delay_Us( Delay200uS_count ); // 200uS delay

    lcd_cmd( 0x38 );              // function set
    lcd_cmd( 0x0C );              // Display on/off control, cursor blink off (0x0C)
    lcd_cmd( 0x06 );			  // entry mode set (0x06)
    Delay(Delay_15mS_Cnt);
    Delay(Delay_15mS_Cnt);
    Delay(Delay_15mS_Cnt);
}

///////////////////////////////////////////////////////////////////
/// lcd_cmd
/// @param cmd Command data byte
///////////////////////////////////////////////////////////////////
void lcd_cmd( char cmd )          // subroutiune for lcd commands
{
	DATA &= 0xFF00;               // prepare RE0 - RE7
    DATA |= cmd;                  // command byte to lcd
	RW = 0;                       // ensure RW is 0
    RS = 0;
    E = 1;                        // toggle E line
    Nop();
    Nop();
    Nop();
    E = 0;
   	Delay(Delay_5mS_Cnt);         // 5ms delay
}

///////////////////////////////////////////////////////////////////
/// lcd_data
/// @param data Output data byte
///////////////////////////////////////////////////////////////////
void lcd_data( char data )        // subroutine for lcd data
{
	RW = 0;       				 // ensure RW is 0
    RS = 1;                       // assert register select to 1
	DATA &= 0xFF00;               // prepare RE0 - RE7
    DATA |= data;                 // data byte to lcd
    E = 1;				
 	Nop();
    Nop();
    Nop();
    E = 0;                       // toggle E signal
    RS = 0;                      // negate register select to 0
    Delay_Us( Delay200uS_count ); // 200uS delay
    Delay_Us( Delay200uS_count ); // 200uS delay
}

///////////////////////////////////////////////////////////////////
/// puts_lcd
/// @param data Pointer to output data array
/// @param count Number of bytes of output data
///////////////////////////////////////////////////////////////////void puts_lcd( unsigned char *data, unsigned char count ) 
{
  	while ( count )
	{
		lcd_data( *data++ );
		count--;
	}	
}

