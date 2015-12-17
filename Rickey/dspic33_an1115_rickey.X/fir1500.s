; ..............................................................................
;    File   fir1500.s
; ..............................................................................

		.equ fir1500NumTaps, 167

; ..............................................................................
; Allocate and initialize filter taps

		.section .xdata, xmemory, data
		.align 512

fir1500Taps:
.hword  0xFFFE, 0xFFFD, 0xFFFC, 0xFFFA, 0xFFF8, 0xFFF6, 0xFFF4, 0xFFF1, 0xFFF0
.hword  0xFFEF, 0xFFEE, 0xFFEF, 0xFFF1, 0xFFF4, 0xFFF8, 0xFFFE, 0x3, 0xB
.hword  0x13, 0x1C, 0x24, 0x2C, 0x32, 0x36, 0x38, 0x37, 0x32
.hword  0x2A, 0x1F, 0x11, 0x0, 0xFFEE, 0xFFDA, 0xFFC7, 0xFFB5, 0xFFA5
.hword  0xFF9A, 0xFF93, 0xFF93, 0xFF9A, 0xFFA7, 0xFFBC, 0xFFD8, 0xFFF9, 0x1E
.hword  0x46, 0x6E, 0x94, 0xB6, 0xD0, 0xD8, 0xE5, 0xDC, 0xC5
.hword  0xA0, 0x6C, 0x2C, 0xFFE3, 0xFF93, 0xFF40, 0xFEEE, 0xFEA4, 0xFE66
.hword  0xFE3A, 0xFE24, 0xFE29, 0xFE4C, 0xFE90, 0xFEF6, 0xFF7D, 0x24, 0xE5
.hword  0x1C5, 0x2B5, 0x3B1, 0x4B0, 0x5AC, 0x69C, 0x777, 0x836, 0x8D3
.hword  0x947, 0x98E, 0x9A6, 0x98E, 0x947, 0x8D3, 0x836, 0x777, 0x69C
.hword  0x5AC, 0x4B0, 0x3B1, 0x2B5, 0x1C5, 0xE5, 0x24, 0xFF7D, 0xFEF6
.hword  0xFE90, 0xFE4C, 0xFE29, 0xFE24, 0xFE3A, 0xFE66, 0xFEA4, 0xFEEE, 0xFF40
.hword  0xFF93, 0xFFE3, 0x2C, 0x6C, 0xA0, 0xC5, 0xDC, 0xE5, 0xD8
.hword  0xD0, 0xB6, 0x94, 0x6E, 0x46, 0x1E, 0xFFF9, 0xFFD8, 0xFFBC
.hword  0xFFA7, 0xFF9A, 0xFF93, 0xFF93, 0xFF9A, 0xFFA5, 0xFFB5, 0xFFC7, 0xFFDA
.hword  0xFFEE, 0x0, 0x11, 0x1F, 0x2A, 0x32, 0x37, 0x38, 0x36
.hword  0x32, 0x2C, 0x24, 0x1C, 0x13, 0xB, 0x3, 0xFFFE, 0xFFF8
.hword  0xFFF4, 0xFFF1, 0xFFEF, 0xFFEE, 0xFFEF, 0xFFF0, 0xFFF1, 0xFFF4, 0xFFF6
.hword  0xFFF8, 0xFFFA, 0xFFFC, 0xFFFD, 0xFFFE

; ..............................................................................
; Allocate delay line in (uninitialized) Y data space

		.section .ybss, ymemory, bss
		.align 512

fir1500DelayI:
		.space fir1500NumTaps*2

		.align 512

fir1500DelayQ:
		.space fir1500NumTaps*2

; ..............................................................................
; Allocate and intialize filter structure

		.section .data
		.global _fir1500FilterI

_fir1500FilterI:
.hword fir1500NumTaps
.hword fir1500Taps
.hword fir1500Taps+fir1500NumTaps*2-1
.hword 0xff00
.hword fir1500DelayI
.hword fir1500DelayI+fir1500NumTaps*2-1
.hword fir1500DelayI

		.global _fir1500FilterQ

_fir1500FilterQ:
.hword fir1500NumTaps
.hword fir1500Taps
.hword fir1500Taps+fir1500NumTaps*2-1
.hword 0xff00
.hword fir1500DelayQ
.hword fir1500DelayQ+fir1500NumTaps*2-1
.hword fir1500DelayQ

; ..............................................................................
; ..............................................................................
; Sample assembly language calling program
;  The following declarations can be cut and pasted as needed into a program
;		.extern	_FIRFilterInit
;		.extern	_BlockFIRFilter
;		.extern	_fir1500Filter
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
;		mov	#_fir1500Filter, W0	; Initalize W0 to filter structure
;		call	_FIRFilterInit	; call this function once
;
; The next 4 instructions are required prior to each subroutine call
; to _BlockFIRFilter
;		mov	#_fir1500Filter, W0	; Initalize W0 to filter structure
;		mov	#input, W1	; Initalize W1 to input buffer 
;		mov	#output, W2	; Initalize W2 to output buffer
;		mov	#20, W3	; Initialize W3 with number of required output samples
;		call	_BlockFIRFilter	; call as many times as needed
