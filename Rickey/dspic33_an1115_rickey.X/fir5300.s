; ..............................................................................
;    File   fir5300.s
; ..............................................................................

		.equ fir5300NumTaps, 85

; ..............................................................................
; Allocate and initialize filter taps

		.section .xdata, xmemory, data
		.align 256

fir5300Taps:
.hword 	0x0002,	0x0002,	0x0003,	0x0005,	0x0008,	0x000B,	0x000F,	0x0015,	0x001B
.hword 	0x0023,	0x002D,	0x0038,	0x0045,	0x0054,	0x0066,	0x007A,	0x0090,	0x00A9
.hword 	0x00C4,	0x00E2,	0x0102,	0x0124,	0x0149,	0x0170,	0x0198,	0x01C2,	0x01ED
.hword 	0x0219,	0x0244,	0x0270,	0x029B,	0x02C6,	0x02EE,	0x0314,	0x0338,	0x0359
.hword 	0x0377,	0x0390,	0x03A6,	0x03B7,	0x03C3,	0x03CB,	0x03CD,	0x03CB,	0x03C3
.hword 	0x03B7,	0x03A6,	0x0390,	0x0377,	0x0359,	0x0338,	0x0314,	0x02EE,	0x02C6
.hword 	0x029B,	0x0270,	0x0244,	0x0219,	0x01ED,	0x01C2,	0x0198,	0x0170,	0x0149
.hword 	0x0124,	0x0102,	0x00E2,	0x00C4,	0x00A9,	0x0090,	0x007A,	0x0066,	0x0054
.hword 	0x0045,	0x0038,	0x002D,	0x0023,	0x001B,	0x0015,	0x000F,	0x000B,	0x0008
.hword 	0x0005,	0x0003,	0x0002,	0x0002

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
