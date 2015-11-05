; ..............................................................................
;    File   fir1500.s
; ..............................................................................

		.equ fir1500NumTaps, 167

; ..............................................................................
; Allocate and initialize filter taps

		.section .xdata, xmemory, data
		.align 512

fir1500Taps:
.hword 	0xFFFE,	0xFFFD,	0xFFFC,	0xFFFA,	0xFFF8,	0xFFF6,	0xFFF4,	0xFFF1,	0xFFF0
.hword 	0xFFEF,	0xFFEE,	0xFFEF,	0xFFF1,	0xFFF4,	0xFFF8,	0xFFFE,	0x0003,	0x000B
.hword 	0x0013,	0x001C,	0x0024,	0x002C,	0x0032,	0x0036,	0x0038,	0x0037,	0x0032
.hword 	0x002A,	0x001F,	0x0011,	0x0000,	0xFFEE,	0xFFDA,	0xFFC7,	0xFFB5,	0xFFA5
.hword 	0xFF9A,	0xFF93,	0xFF93,	0xFF9A,	0xFFA7,	0xFFBC,	0xFFD8,	0xFFF9,	0x001E
.hword 	0x0046,	0x006E,	0x0094,	0x00B6,	0x00D0,	0x00E0,	0x00E5,	0x00DC,	0x00C5
.hword 	0x00A0,	0x006C,	0x002C,	0xFFE3,	0xFF93,	0xFF40,	0xFEEE,	0xFEA4,	0xFE66
.hword 	0xFE3A,	0xFE24,	0xFE29,	0xFE4C,	0xFE90,	0xFEF6,	0xFF7D,	0x0024,	0x00E8
.hword 	0x01C5,	0x02B5,	0x03B1,	0x04B0,	0x05AC,	0x069C,	0x0777,	0x0836,	0x08D3
.hword 	0x0947,	0x098E,	0x09A6,	0x098E,	0x0947,	0x08D3,	0x0836,	0x0777,	0x069C
.hword 	0x05AC,	0x04B0,	0x03B1,	0x02B5,	0x01C5,	0x00E8,	0x0024,	0xFF7D,	0xFEF6
.hword 	0xFE90,	0xFE4C,	0xFE29,	0xFE24,	0xFE3A,	0xFE66,	0xFEA4,	0xFEEE,	0xFF40
.hword 	0xFF93,	0xFFE3,	0x002C,	0x006C,	0x00A0,	0x00C5,	0x00DC,	0x00E5,	0x00E0
.hword 	0x00D0,	0x00B6,	0x0094,	0x006E,	0x0046,	0x001E,	0xFFF9,	0xFFD8,	0xFFBC
.hword 	0xFFA7,	0xFF9A,	0xFF93,	0xFF93,	0xFF9A,	0xFFA5,	0xFFB5,	0xFFC7,	0xFFDA
.hword 	0xFFEE,	0x0000,	0x0011,	0x001F,	0x002A,	0x0032,	0x0037,	0x0038,	0x0036
.hword 	0x0032,	0x002C,	0x0024,	0x001C,	0x0013,	0x000B,	0x0003,	0xFFFE,	0xFFF8
.hword 	0xFFF4,	0xFFF1,	0xFFEF,	0xFFEE,	0xFFEF,	0xFFF0,	0xFFF1,	0xFFF4,	0xFFF6
.hword 	0xFFF8,	0xFFFA,	0xFFFC,	0xFFFD,	0xFFFE

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
