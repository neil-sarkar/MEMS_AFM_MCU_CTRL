;***********************************************************************
;                                                                      * 
;     Author:         Darren Wenn                                      *
;     Company:        Microchip Ltd                                    * 
;     Filename:       foldedFIR.s                                      *
;     Date:           01/11/2006                                       *
;     File Version:   1.10                                             *
;     Other Files Required: project files			                   *
;                                                                      *
;***********************************************************************
/* SOFTWARE LICENSE AGREEMENT:
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
* V1.0  D.Wenn		High efficiency folded FIR implementation
*					with decimation and fc*4 sampling
* V1.1	D.Wenn		Improved algorithm to use half rate filters
*
********************************************************************/

; include common definitions
	.nolist
	.include	"dspcommon.inc"
	.list

	.section .dspExtensions, code
	
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;	_quadratureMult: Multiply an input signal sampled at 4X the
;			carrier frequency by both cosine and sine signals.
;			Also known as quadrature sampling with digital mixing
;			Since we are sampling at 4 x fc then the i(n) signal
;			is generated by multiplying the input x(n) by 1,0,-1,0
;			and q(n) by 0,-1,0,1. Which can be expressed without 
;			fractional multiplies.
;			Refinement is to ignore the 0 values and not place them
;			into the destination arrays. This technique effectively
;			allows the following FIR filters to be run at half the
;			sample rate (since every other term is zero anyway)
;			Apply the DC offset to the samples
;
;	W0 = N, number of sample points
;	w1 = x(n), pointer to input sample array
;	w2 = i(n), pointer to location for i(n) data to be placed
;	w3 = q(n), pointer to location for q(n) data to be placed
;	w4 = DCOffset
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	.global _quadratureMult
	
; this version is 3 + 8 * (N/4) = 83 cycles for 40 elements
_quadratureMult:
	lsr		w0, #2, w0		; divide number of samples by 4
	dec		w0, w0			; adjust for number of data entries
	do		w0, _endquadMult
	add		w4, [w1++], [w2++]	; i(n) = 1 * x(n) + DCOffset
	add		w4, [w1], [w1]
	neg		[w1++], [w3++]	; q(n+1) = -1 * x(n+1)
	add		w4, [w1], [w1]
	neg		[w1++], [w2++]	; i(n+2) = -1 * x(n+2)
_endquadMult:
	add		w4, [w1++], [w3++]	; q(n+3) = 1 * x(n+3) + DCOffset

	return


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;	_foldedFIR: Perform a folded FIR. This code shifts in a new
;			batch of data into the delay buffer and then performs
;			a FIR over the data finally decimating the data by the
;			sample size and returning a single value. The coefficients
;			can be in program memory or RAM. The delay buffer is
;			assumed to be an increasing circular buffer and the
;			number of samples passed in is assumed to be less than
;			the number of coefficients
;Inputs
;	W0 = N, number of samples
;	W1 = x(n) input samples
;	W2 = address of filter structure
;	Delay buffer is defined as increasing circular buffer in Y space
;Output
;	W0 = y new output value
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	.global _foldedFIR
_foldedFIR:
	; save working registers
	push	w8
	push	w10

	; prepare CORCON for fractional computation
	push	CORCON
	fractsetup w8
	
	; prepare CORCON and PSVPAG to access data in program memory
	push	PSVPAG
	mov		[w2 + oCoeffsPage], w10	; w10 = coefficients page
	mov		#COEFFS_IN_DATA, w8		; w8 = COEFFS_IN_DATA
	cp		w8, w10
	bra		z, _noPSV
	psvaccess w8					; enable PSV bit in CORCON
	mov		w10, PSVPAG				; load PSVPAG with program space offset

_noPSV:		
	push	MODCON
	push	YMODSRT
	push	YMODEND

	; setup registers for modulo addressing
	; Y modulo addressing will be used for the delay buffer
	; 
	mov		#0x40A0, w10			; YWM = w10, enable Y Modulo
	mov		w10, MODCON	
	mov		[w2 + oDelayEnd], w10	; YMODEND = end of delay buffer
	mov		w10, YMODEND
	mov		[w2 + oDelayBase], w10 	; YMODSRT = start of delay buffer
	mov		w10, YMODSRT
	mov		[w2 + oDelay], w10		; w10 = current delay entry point
	
	mov		[W2 + oCoeffsBase], w8	; w8 = h(0) 
	mov		[w2 + oNumCoeffs], w4	; w4 = M
	dec		w4, w4					; w4 = M - 1
	dec		w0, w0					; w0 = N - 1
	
	; copy the new data into the delay buffer 
	; overwriting the oldest entries as we go
	repeat	w0
	mov		[w1++], [w10++]
.ifdef YMEM_ERRATA
	nop
.endif
	
	; since w10 is now pointing to the oldest entry we
	; can now process the entire data set	
	; clear ACCA and preload w5 and w6 with first coeff and data
	clr		a, [w8]+=2, w5, [w10]+=2, w6
	
	repeat	w4						; repeat M times
	mac		w5*w6, a, [w8]+=2, w5, [w10]+=2, w6
	
_endFilter:
	sac.r	a, w0	
	mov		w10, [w2 + oDelay]		; preserve current entry point
	
	; restore modulo and paging registers
	pop		YMODEND
	pop 	YMODSRT
	pop 	MODCON
	pop		PSVPAG
	pop		CORCON
	; restore working registers
	pop		w10
	pop		w8
	
	return
	
	.end
	
		
