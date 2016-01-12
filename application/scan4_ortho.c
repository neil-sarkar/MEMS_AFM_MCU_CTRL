#include "scan4_ortho.h"

#ifdef configMEMS_4ACT_ORTHO

extern u8 isPidOn;
extern u16 pid_input;
extern u16 pid_phase;

struct leveling
{
	dac ch;
	u16 val;
	u16 delta;
	u8  dir;
};

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

struct scan4_o 
{
	u16 	numPts;
	u16 	numLines;
	u16 	data;
	u16 	xIncrement;
	u16 	yIncrement;
	u16		xStepCnt;
	u16 	lineCnt;
	u8		sendBackCnt;
	u8  	sampleCnt;
	u8  	dwellTime_ms;
	u8		iLine;
	bool   	isDirFWD;
	bool	start;
	bool	isLastPnt; 
	struct 	flash f;
	struct 	act_pair xp;
	struct 	act_pair yp;
	struct 	img_data img;
};

static struct scan4_o s4_o;
static struct leveling lvl;	

#define GET_DAC_VAL(index) flash_Read2BytesBlk0((BLOCK0_BASE + (index << 1)) & BLOCK_MASK, &(s4_o.data));	
																					  		
__inline static void SET_PAIRX(u16 v1, u16 v2)
{	
	s4_o.xp.a1Val = v1;
	s4_o.xp.a2Val = v2;
	GET_DAC_VAL(v1); 													
	dac_set_val(s4_o.xp.a1, s4_o.data);											
	GET_DAC_VAL(v2); 													
	dac_set_val(s4_o.xp.a2, s4_o.data);
}

__inline static void SET_PAIRY(u16 v1, u16 v2)
{	
	s4_o.yp.a1Val = v1;
	s4_o.yp.a2Val = v2;
	GET_DAC_VAL(v1);															
	dac_set_val(s4_o.yp.a1, s4_o.data);											
	GET_DAC_VAL(v2);															
	dac_set_val(s4_o.yp.a2, s4_o.data);
}

#define SCAN_PAIRX_FWD 		SET_PAIRX((s4_o.xp.a1Val - s4_o.xIncrement), (s4_o.xp.a2Val + s4_o.xIncrement));

#define SCAN_PAIRX_BWD 		SET_PAIRX((s4_o.xp.a1Val + s4_o.xIncrement), (s4_o.xp.a2Val - s4_o.xIncrement));

#define SCAN_NEXT_LINE		SET_PAIRY((s4_o.yp.a1Val - s4_o.yIncrement), (s4_o.yp.a2Val + s4_o.yIncrement));

#define SAMPLE_WAIT_CNT_ABS	50

#define TAKE_MEASUREMENT	if (isPidOn == 1)																	\
							{																					\
								s4_o.img.zAmplitude = pid_input;												\
								s4_o.img.zOffset = dac_get_val(DAC_ZOFFSET_FINE);			 					\
								s4_o.img.zPhase = pid_phase;													\
							}																					\
							else																				\
							{																					\
								adc_start_conv (ADC_ZAMP);	 													\
								s4_o.img.zAmplitude = adc_get_avgw_val(s4_o.sampleCnt, SAMPLE_WAIT_CNT_ABS); 	\
								s4_o.img.zOffset = dac_get_val(DAC_ZOFFSET_FINE);			 					\
								adc_start_conv(ADC_PHASE);								 						\
								s4_o.img.zPhase = adc_get_avgw_val(s4_o.sampleCnt, SAMPLE_WAIT_CNT_ABS);		\
							}

#define SEND_DATA_TO_CLIENT uart_set_char((s4_o.img.zAmplitude & 0xFF)); 										\
							uart_set_char((s4_o.img.zAmplitude >> 8) & 0x0F); 									\
							uart_set_char(s4_o.img.zOffset & 0xFF); 											\
							uart_set_char((s4_o.img.zOffset >> 8) & 0x0F); 										\
							uart_set_char(s4_o.img.zPhase & 0xFF); 												\
							uart_set_char((s4_o.img.zPhase >> 8) & 0x0F);				

#define SCAN				if (s4_o.isDirFWD)																	\
							{														  							\
								SCAN_PAIRX_FWD;																	\
							}	 																				\
							else																				\
							{																					\
								SCAN_PAIRX_BWD;																	\
							}
							
#define SCAN_LAST			SET_PAIRX(s4_o.xp.a1Val, s4_o.xp.a2Val);											\
		 					SET_PAIRY(s4_o.yp.a1Val, s4_o.yp.a2Val)							

#define SWAP_DIRECTION		if (s4_o.isDirFWD)																	\
								s4_o.isDirFWD = false;															\
							else				  																\
								s4_o.isDirFWD = true;

#define LEVEL				if (lvl.dir == 'f')																	\
							{																					\
								dac_set_val(lvl.ch, (lvl.val += lvl.delta));								   	\
							}																				   	\
							else																				\
							{																					\
								dac_set_val(lvl.ch, (lvl.val -= lvl.delta));									\
							}
				  
void scan4_ortho_start (void)
{
	s4_o.start 			= true;
	s4_o.isDirFWD 		= true;
	s4_o.isLastPnt		= false;
	s4_o.xStepCnt 		= 0;
	s4_o.lineCnt 		= 0;
	s4_o.iLine			= 0;

	s4_o.xp.a1StartVal 	= 4095;
	s4_o.xp.a2StartVal 	= 0;

	s4_o.yp.a1StartVal 	= 4095;
	s4_o.yp.a2StartVal	= 0;

	SET_PAIRX(s4_o.xp.a1StartVal, s4_o.xp.a2StartVal);
	SET_PAIRY(s4_o.yp.a1StartVal, s4_o.yp.a2StartVal);

	if (lvl.dir == 'f')
	{
		dac_set_val(lvl.ch, (lvl.val = 0));
	}
	else
	{
		dac_set_val(lvl.ch, (lvl.val = 4095));
	}

	// this is a wait so that the actuators settle at the starting point
	// before starting the scan
	DELAY_MS(10);
}

void scan4_ortho_step (void)
{
	u8 i;
	s4_o.isLastPnt = false;

	// this loop keeping track of the number of lines that need to be scanned
	// this is essentially the whole scan proccess
	for ( ; s4_o.lineCnt < s4_o.numLines; s4_o.lineCnt++)
	{
		// scan one line back and forth
		for ( ; s4_o.iLine < 2; s4_o.iLine++)
		{
			for ( ; s4_o.xStepCnt < s4_o.numPts;)
			{
				// send data to client
				for (i = 0; i < s4_o.sendBackCnt; i++)
				{
					// If it's the very beginning of the scan, the actuators are already at the start point
					// Take measurement and continue
					if (!s4_o.start)
					{
						if (!s4_o.isLastPnt)
						{
							SCAN;
						}
						else
						{
							SCAN_LAST;
							s4_o.isLastPnt = false;
						}
					}
					else
					{
						s4_o.start = false;
					}

					// dwell
					DELAY_MS(s4_o.dwellTime_ms);

					// take all measurements
					//pid_wait_update ();		// TODO: is this necessary????
					TAKE_MEASUREMENT;
					// send data to client
					SEND_DATA_TO_CLIENT;
					s4_o.xStepCnt++;		
				}
				return;
			}
			// one scan forward/backward completed
			s4_o.xStepCnt = 0;
			s4_o.isLastPnt = true;
			SWAP_DIRECTION;
		}
		s4_o.iLine = 0;
		SCAN_NEXT_LINE;
		LEVEL;
	}
	// reset for the next scan
	s4_o.lineCnt = 0;
}

void scan4_ortho_init (void)
{
	s4_o.f.adr  		= BLOCK0_BASE;
	s4_o.xp.a1  		= DAC_X2;
	s4_o.xp.a2  		= DAC_X1;

	s4_o.yp.a1  		= DAC_Y2;
	s4_o.yp.a2  		= DAC_Y1;

	lvl.ch				= DAC_BFRD3;
	lvl.dir 		 	= 'f';

	s4_o.xStepCnt 		= 0;
	s4_o.lineCnt 		= 0;
	s4_o.dwellTime_ms 	= 1;
	s4_o.sampleCnt 		= 16; 
	s4_o.sendBackCnt = 8;
}

void s4_set_dwell_t_ms (u8 dwell_ms)
{
	s4_o.dwellTime_ms = dwell_ms;	
}

void s4_set_sample_cnt (u8 sample_cnt)
{
	s4_o.sampleCnt = sample_cnt;
}

/* terminal interaction */
void scan4_ortho_get_data (void)
{
	u8 val_l, val_h;

	val_l = uart_wait_get_char();
	val_h = uart_wait_get_char();
	s4_o.numPts = (val_h << 8) | val_l;

	val_l = uart_wait_get_char();
	val_h = uart_wait_get_char();
	s4_o.numLines = (val_h << 8) | val_l;

	s4_o.xIncrement = 4096 / s4_o.numPts;
	s4_o.yIncrement = 4096 / s4_o.numLines;

	lvl.delta		= 4095 / s4_o.numLines;
}

bool scan4_ortho_get_dac_data (void)
{
	u16 i;
	u8 val_l, val_h;

	for (i = 0; i < PAGE_SIZE/2; i ++)
	{
		val_l = uart_wait_get_char();
		val_h = (uart_wait_get_char() & 0x0F); 
		s4_o.f.buffer[i] = (val_h << 8) | val_l;	
	}

	if (flash_EraseSector (s4_o.f.adr))
		return false;
	if (flash_WriteAdr (s4_o.f.adr, PAGE_SIZE, (u8*)s4_o.f.buffer))
		return false;
	s4_o.f.adr += PAGE_SIZE;
	
	return true;	
}

void s4_set_send_back_cnt (u8 send_back_cnt)
{
	s4_o.sendBackCnt = send_back_cnt;	
}

void s4_set_lvl_dir (u8 dir)
{
	lvl.dir = dir;	
}

#endif
