; ..............................................................................
;    File   fir150.s
; ..............................................................................

		.equ fir150NumTaps, 53

; ..............................................................................
; Allocate and initialize filter taps

		.section .xdata, xmemory, data
		.align 128

fir150Taps:
.hword 	0xFFFE,	0xFFFA,	0xFFF3,	0xFFEA,	0xFFDD,	0xFFCE,	0xFFBF,	0xFFB0,	0xFFA6
.hword 	0xFFA4,	0xFFAF,	0xFFCD,	0x0002,	0x0054,	0x00C5,	0x0158,	0x020B,	0x02DB
.hword 	0x03C2,	0x04B8,	0x05B1,	0x06A1,	0x077D,	0x0837,	0x08C4,	0x091D,	0x093B
.hword 	0x091D,	0x08C4,	0x0837,	0x077D,	0x06A1,	0x05B1,	0x04B8,	0x03C2,	0x02DB
.hword 	0x020B,	0x0158,	0x00C5,	0x0054,	0x0002,	0xFFCD,	0xFFAF,	0xFFA4,	0xFFA6
.hword 	0xFFB0,	0xFFBF,	0xFFCE,	0xFFDD,	0xFFEA,	0xFFF3,	0xFFFA,	0xFFFE

; ..............................................................................
; Allocate delay line in (uninitialized) Y data space

		.section .ybss, ymemory, bss
		.align 128

fir150DelayI:
		.space fir150NumTaps*2

		.align 128

fir150DelayQ:
		.space fir150NumTaps*2
; ..............................................................................
; Allocate and intialize filter structure

		.section .data
		.global _fir150FilterI

_fir150FilterI:
.hword fir150NumTaps
.hword fir150Taps
.hword fir150Taps+fir150NumTaps*2-1
.hword 0xff00
.hword fir150DelayI
.hword fir150DelayI+fir150NumTaps*2-1
.hword fir150DelayI

		.global _fir150FilterQ

_fir150FilterQ:
.hword fir150NumTaps
.hword fir150Taps
.hword fir150Taps+fir150NumTaps*2-1
.hword 0xff00
.hword fir150DelayQ
.hword fir150DelayQ+fir150NumTaps*2-1
.hword fir150DelayQ

; ..............................................................................
; ..............................................................................
; Sample assembly language calling program
;  The following declarations can be cut and pasted as needed into a program
;		.extern	_FIRFilterInit
;		.extern	_BlockFIRFilter
;		.extern	_fir150Filter
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
;		mov	#_fir150Filter, W0	; Initalize W0 to filter structure
;		call	_FIRFilterInit	; call this function once
;
; The next 4 instructions are required prior to each subroutine call
; to _BlockFIRFilter
;		mov	#_fir150Filter, W0	; Initalize W0 to filter structure
;		mov	#input, W1	; Initalize W1 to input buffer 
;		mov	#output, W2	; Initalize W2 to output buffer
;		mov	#20, W3	; Initialize W3 with number of required output samples
;		call	_BlockFIRFilter	; call as many times as needed
