; ..............................................................................
;    File   fir5300.s
; ..............................................................................

		.equ fir5300NumTaps, 85

; ..............................................................................
; Allocate and initialize filter taps

		.section .xdata, xmemory, data
		.align 256

fir5300Taps:
.hword  0x2, 0x2, 0x3, 0x5, 0x8, 0xB, 0xF, 0x15, 0x1B
.hword  0x23, 0x2D, 0x38, 0x45, 0x54, 0x66, 0x7A, 0x90, 0xA9
.hword  0xC4, 0xE5, 0x102, 0x124, 0x149, 0x170, 0x198, 0x1C2, 0x1ED
.hword  0x219, 0x244, 0x270, 0x29B, 0x2C6, 0x2EE, 0x314, 0x338, 0x359
.hword  0x377, 0x390, 0x3A6, 0x3B7, 0x3C3, 0x3CB, 0x3CD, 0x3CB, 0x3C3
.hword  0x3B7, 0x3A6, 0x390, 0x377, 0x359, 0x338, 0x314, 0x2EE, 0x2C6
.hword  0x29B, 0x270, 0x244, 0x219, 0x1ED, 0x1C2, 0x198, 0x170, 0x149
.hword  0x124, 0x102, 0xE5, 0xC4, 0xA9, 0x90, 0x7A, 0x66, 0x54
.hword  0x45, 0x38, 0x2D, 0x23, 0x1B, 0x15, 0xF, 0xB, 0x8
.hword  0x5, 0x3, 0x2, 0x2

; ..............................................................................
; Allocate delay line in (uninitialized) Y data space

		.section .ybss, ymemory, bss
		.align 256

fir5300DelayI:
		.space fir5300NumTaps*2

		.align 256

fir5300DelayQ:
		.space fir5300NumTaps*2
		
; ..............................................................................
; Allocate and intialize filter structure

		.section .data
		.global _fir5300FilterI

_fir5300FilterI:
.hword fir5300NumTaps
.hword fir5300Taps
.hword fir5300Taps+fir5300NumTaps*2-1
.hword 0xff00
.hword fir5300DelayI
.hword fir5300DelayI+fir5300NumTaps*2-1
.hword fir5300DelayI

		.global _fir5300FilterQ

_fir5300FilterQ:
.hword fir5300NumTaps
.hword fir5300Taps
.hword fir5300Taps+fir5300NumTaps*2-1
.hword 0xff00
.hword fir5300DelayQ
.hword fir5300DelayQ+fir5300NumTaps*2-1
.hword fir5300DelayQ

; ..............................................................................
; ..............................................................................
; Sample assembly language calling program
;  The following declarations can be cut and pasted as needed into a program
;		.extern	_FIRFilterInit
;		.extern	_BlockFIRFilter
;		.extern	_fir5300Filter
;
;		.section	.bss
;
;	 The input and output buffers can be made any desired size
;	   the value 40 is just an example - however, one must ensure
;	   that the output buffer is at least as long as the number of samples
;	   to be filtered (parameter 4)
;input:		.space	40
;output:	.space	40
;		.text
;
;
;  This code can be copied and pasted as needed into a program
;
;
; Set up pointers to access input samples, filter taps, delay line and
; output samples.
;		mov	#_fir5300Filter, W0	; Initalize W0 to filter structure
;		call	_FIRFilterInit	; call this function once
;
; The next 4 instructions are required prior to each subroutine call
; to _BlockFIRFilter
;		mov	#_fir5300Filter, W0	; Initalize W0 to filter structure
;		mov	#input, W1	; Initalize W1 to input buffer 
;		mov	#output, W2	; Initalize W2 to output buffer
;		mov	#20, W3	; Initialize W3 with number of required output samples
;		call	_BlockFIRFilter	; call as many times as needed
