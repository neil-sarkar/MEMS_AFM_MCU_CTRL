#include "scan4.h"

#ifdef configMEMS_4ACT

extern u8 isPidOn;
extern u16 pid_input;
extern u16 pid_phase;
extern u16 pid_liamp1;
extern u16 pid_liamp2;

struct flash
{
	u32 adr;
	u16 buffer[PAGE_SIZE/2];		
};

struct act_pair
{
	dac a1;
	u16 a1Val;
	u16 a1StartVal;

	dac a2;
	u16 a2Val;
	u16 a2StartVal;
};

struct img_data
{
	u16 zAmplitude;
	u16 zOffset;
	u16 zPhase;		
};

struct leveling
{
	dac ch;
	u16 val;
	u16 delta;
	u8  dir;
};

struct scan4 
{
	u8  	ratio;
	u16 	numPts;
	u16 	numLines;
	u16 	data;
	u16 	xIncrement;
	u16 	yIncrement;
	u16 	xRange;
	u16		xStepCnt;
	u16 	yRange;
	u16 	lineCnt;
	u16		sendBackCnt;
	u16		liamp1;
	u16 	liamp2;
	u8  	sampleCnt;
	u8  	dwellTime_ms;
	u8		iLine;
	bool   	isXScanDirDwn;
	bool	start;
	bool	isLastPnt; 
	struct 	flash f;
	struct 	act_pair xp;
	struct 	act_pair yp;
	struct 	img_data img;
	struct 	leveling lvl;
};

struct scan4 s4;

#define GET_DAC_VAL(index) flash_Read2BytesBlk0((BLOCK0_BASE + (index << 1)) & BLOCK_MASK, &(s4.data));	
																					  		
__inline static void SET_PAIRX(u16 v1, u16 v2)
{	
	s4.xp.a1Val = v1;
	s4.xp.a2Val = v2;
	GET_DAC_VAL(v1); 													
	dac_set_val(s4.xp.a1, s4.data);											
	GET_DAC_VAL(v2); 													
	dac_set_val(s4.xp.a2, s4.data);
}

__inline static void SET_PAIRY(u16 v1, u16 v2)
{	
	s4.yp.a1Val = v1;
	s4.yp.a2Val = v2;
	GET_DAC_VAL(v1);															
	dac_set_val(s4.yp.a1, s4.data);											
	GET_DAC_VAL(v2);															
	dac_set_val(s4.yp.a2, s4.data);
}

#define SCAN_PAIRX_DWN 		SET_PAIRX((s4.xp.a1Val - s4.xIncrement), (s4.xp.a2Val + s4.xIncrement));

#define SCAN_PAIRX_UP 		SET_PAIRX((s4.xp.a1Val + s4.xIncrement), (s4.xp.a2Val - s4.xIncrement));

#define SCAN_PAIRY_DWN 		SET_PAIRY((s4.yp.a1Val - s4.xIncrement), (s4.yp.a2Val + s4.xIncrement));

#define SCAN_PAIRY_UP 		SET_PAIRY((s4.yp.a1Val + s4.xIncrement), (s4.yp.a2Val - s4.xIncrement));

#define SCAN_NEXT_LINE		SET_PAIRX((s4.xp.a1StartVal -= s4.yIncrement), s4.xp.a2StartVal += s4.yIncrement); 	\
							SET_PAIRY((s4.yp.a1StartVal -= s4.yIncrement), s4.yp.a2StartVal += s4.yIncrement) 			// TODO: why is the start val being set here?

#define SAMPLE_WAIT_CNT_ABS	50

#define TAKE_MEASUREMENT	if (isPidOn == 1)																									\
							{																																							\
								s4.img.zAmplitude = pid_input;																							\
								s4.img.zOffset = dac_get_val(DAC_ZOFFSET_FINE);			 												\
								s4.img.zPhase = pid_phase;																									\
								s4.liamp1 = pid_liamp1;																											\
								s4.liamp2 = pid_liamp2;																											\
							}																																							\
							else																																					\
							{																																							\
								adc_start_conv (ADC_ZAMP);	 																								\
								s4.img.zAmplitude = adc_get_avgw_val(s4.sampleCnt, SAMPLE_WAIT_CNT_ABS); 		\
								s4.img.zOffset = dac_get_val(DAC_ZOFFSET_FINE);			 												\
								adc_start_conv(ADC_PHASE);								 																	\
								s4.img.zPhase = adc_get_avgw_val(s4.sampleCnt, SAMPLE_WAIT_CNT_ABS);				\
								adc_start_conv(ADC_LIAMP1);								 																	\
								s4.liamp1 = adc_get_val();																									\
								adc_start_conv(ADC_LIAMP2);								 																	\
								s4.liamp2 = adc_get_val();																									\
							}

#define SEND_DATA_TO_CLIENT uart_set_char((s4.img.zAmplitude & 0xFF)); 											\
							uart_set_char((s4.img.zAmplitude >> 8) & 0x0F); 															\
							uart_set_char(s4.img.zOffset & 0xFF); 																				\
							uart_set_char((s4.img.zOffset >> 8) & 0x0F); 																	\
							uart_set_char(s4.img.zPhase & 0xFF); 																					\
							uart_set_char((s4.img.zPhase >> 8) & 0x0F);																		\
							uart_set_char(s4.liamp1 & 0xFF); 																							\
							uart_set_char((s4.liamp1 >> 8) & 0x0F);																				\
							uart_set_char(s4.liamp2 & 0xFF); 																							\
							uart_set_char((s4.liamp2 >> 8) & 0x0F);																				\

#define SCAN				if (s4.isXScanDirDwn)																\
							{														  							\
								SCAN_PAIRX_DWN;																	\
								SCAN_PAIRY_UP;																	\
							}	 																				\
							else																				\
							{																					\
								SCAN_PAIRX_UP;																	\
								SCAN_PAIRY_DWN;																	\
							}
							
#define SCAN_LAST			SET_PAIRX(s4.xp.a1Val, s4.xp.a2Val);												\
		 					SET_PAIRY(s4.yp.a1Val, s4.yp.a2Val)							

#define SWAP_DIRECTION		if (s4.isXScanDirDwn)																\
								s4.isXScanDirDwn = false;														\
							else				  																\
								s4.isXScanDirDwn = true;

#define SET_LVLING			if (s4.lvl.dir == 'f')																	\
							{																					\
								dac_set_val(s4.lvl.ch, (s4.lvl.val += s4.lvl.delta));							\
							}																					\
							else																				\
							{																					\
								dac_set_val(s4.lvl.ch, (s4.lvl.val -= s4.lvl.delta));							\
							}																					\
			  
void scan4_start (void)
{
	s4.start 				= true;
	s4.isXScanDirDwn 	= true;
	s4.isLastPnt		= false;
	s4.xStepCnt 		= 0;
	s4.lineCnt 			= 0;	
	s4.iLine				= 0;

	s4.xp.a1StartVal 	= 4095;
	s4.xp.a2StartVal 	= 0;

	s4.yp.a1StartVal 	= (s4.yRange - 1);
	s4.yp.a2StartVal	= (s4.xRange - 1);

	SET_PAIRX(s4.xp.a1StartVal, s4.xp.a2StartVal);
	SET_PAIRY(s4.yp.a1StartVal, s4.yp.a2StartVal);

	if (s4.lvl.dir == 'f')
	{
		s4.lvl.val = 0;
	}
	else
	{
		s4.lvl.val = 4095;
	}


	// this is a wait so that the actuators settle at the starting point
	// before starting the scan
	DELAY_MS(50);
}

void scan4_step (void)
{
	u16 i;
	s4.isLastPnt = false;

	// this loop keeping track of the number of lines that need to be scanned
	// this is essentially the whole scan proccess
	for ( ; s4.lineCnt < s4.numLines; s4.lineCnt++)
	{
		// scan one line back and forth
		for ( ; s4.iLine < 2; s4.iLine++)
		{
			for ( ; s4.xStepCnt < s4.numPts;)
			{
				// send data to client
				for (i = 0; i < s4.sendBackCnt; i++)
				{
					if (!s4.start)
					{
						if (!s4.isLastPnt)
						{
							SCAN;
						}
						else
						{
							SCAN_LAST;
							s4.isLastPnt = false;
						}
					}
					else
					{
						s4.start = false;
					}

					// dwell
					DELAY_MS(s4.dwellTime_ms);

					// take all measurements
					//pid_wait_update ();		// TODO: is this necessary????
					TAKE_MEASUREMENT;
					// send data to client
					SEND_DATA_TO_CLIENT;
					s4.xStepCnt++;		
				}
				return;
			}
			// one scan forward/backward completed
			s4.xStepCnt = 0;
			s4.isLastPnt = true;
			SWAP_DIRECTION;
		}
		s4.iLine = 0;
		SCAN_NEXT_LINE;
		SET_LVLING;
	}
	// reset for the next scan
	s4.lineCnt = 0;
}

void scan4_init (void)
{
	s4.f.adr  		= BLOCK0_BASE;
	s4.xp.a1  		= DAC_X2;
	s4.xp.a2  		= DAC_X1;

	s4.yp.a1  		= DAC_Y2;
	s4.yp.a2  		= DAC_Y1;

	s4.lvl.ch		= DAC_BFRD3;
	//default
	s4.lvl.dir		= 'f';

	s4.xStepCnt 	= 0;
	s4.lineCnt 		= 0;
	s4.dwellTime_ms = 1;
	s4.sampleCnt 	= 16;
	
	s4.sendBackCnt	= 8;
}

void s4_set_dwell_t_ms (u8 dwell_ms)
{
	s4.dwellTime_ms = dwell_ms;	
}

void s4_set_sample_cnt (u8 sample_cnt)
{
	s4.sampleCnt = sample_cnt;
}

void scan4_get_data (void)
{
	u8 val_l, val_h;

	val_l = uart_wait_get_char();
	s4.ratio = val_l;

	val_l = uart_wait_get_char();
	val_h = uart_wait_get_char();
	s4.numPts = (val_h << 8) | val_l;

	val_l = uart_wait_get_char();
	val_h = uart_wait_get_char();
	s4.numLines = (val_h << 8) | val_l;
	
	s4.xRange = 4096 / s4.ratio;
	s4.yRange = 4096 - s4.xRange; 
	s4.xIncrement = s4.xRange / s4.numPts;
	s4.yIncrement = s4.yRange / s4.numLines;

	s4.lvl.delta  = 4095 / s4.numLines;
}

/* terminal interaction */
bool scan4_get_dac_data (void)
{
	u16 i;
	u8 val_l, val_h;

	for (i = 0; i < PAGE_SIZE/2; i ++)
	{
		val_l = uart_wait_get_char();
		val_h = (uart_wait_get_char() & 0x0F); 
		s4.f.buffer[i] = (val_h << 8) | val_l;	
	}

	if (flash_EraseSector (s4.f.adr))
		return false;
	if (flash_WriteAdr (s4.f.adr, PAGE_SIZE, (u8*)s4.f.buffer))
		return false;
	s4.f.adr += PAGE_SIZE;

	return true;	
}

void s4_set_send_back_cnt ()
{
	u8 byte_l, byte_h;
	byte_l = uart_wait_get_char();
	byte_h = uart_wait_get_char();
	s4.sendBackCnt = (byte_h << 8) | byte_l;	
}

void s4_set_lvl_dir (u8 dir)
{
	s4.lvl.dir = dir;	
}

#endif
