#include "scan4.h"

#define S4_SEND_CNT 8

struct flash
{
	u32 adr;
	u16 buffer[PAGE_SIZE/2];		
};

struct act_pair
{
	dac a1;
	u16 a1_val;

	dac a2;
	u16 a2_val;
};

struct img_data
{
	u16 zAmplitude;
	u16 zOffset;
	u16 zPhase;		
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
	u8  	sampleCnt;
	u8  	dwellTime_ms;
	u8		j;
	bool   	isXScanDirDwn;
	 
	struct 	flash f;
	struct 	act_pair xp;
	struct 	act_pair yp;
	struct 	img_data img;
};

struct scan4 s4;

static void GET_DAC_VAL(u16 index)
{
	flash_Read2Bytes((BLOCK0_BASE + (index*2)), &(s4.data));
}	
																					  		
#define SET_PAIRX(v1, v2)	s4.xp.a1_val = v1; 															\
							s4.xp.a2_val = v2; 															\
							if (v1 < 0) s4.xp.a1_val = 0;												\
							if (v2 < 0) s4.xp.a2_val = 0;												\
							GET_DAC_VAL(s4.xp.a1_val); 													\
							dac_set_val(s4.xp.a1, s4.data); 											\
							GET_DAC_VAL(s4.xp.a2_val); 													\
							dac_set_val(s4.xp.a2, s4.data);

#define SET_PAIRY(v1, v2)	s4.yp.a1_val = v1; 															\
							s4.yp.a2_val = v2; 															\
							if (v1 < 0) s4.yp.a1_val = 0;												\
							if (v2 < 0) s4.yp.a2_val = 0;												\
							GET_DAC_VAL(v1);															\
							dac_set_val(s4.yp.a1, s4.data); 											\
							GET_DAC_VAL(v2);															\
							dac_set_val(s4.yp.a2, s4.data);

// TODO review this!!!
#define SCAN_PAIRX_DWN		SET_PAIRX((s4.xp.a1_val - s4.xIncrement), (s4.xp.a2_val + s4.xIncrement))

#define SCAN_PAIRX_UP		SET_PAIRX((s4.xp.a1_val + s4.xIncrement), (s4.xp.a2_val - s4.xIncrement))

#define SCAN_PAIRY_DWN		SET_PAIRY((s4.yp.a1_val - s4.xIncrement), (s4.yp.a2_val + s4.xIncrement))

#define SCAN_PAIRY_UP		SET_PAIRY((s4.yp.a1_val + s4.xIncrement), (s4.yp.a2_val - s4.xIncrement))

#define SCAN_NEXT_LINE		SET_PAIRX((s4.xp.a1_val - s4.yIncrement), (s4.xp.a2_val + s4.yIncrement))	\
							SET_PAIRY((s4.yp.a1_val - s4.yIncrement), (s4.yp.a2_val + s4.yIncrement))

#define SAMPLE_WAIT_CNT_ABS	10

#define TAKE_MEASUREMENT	adc_start_conv (ADC_ZAMP);	 												\
							s4.img.zAmplitude = adc_get_avgw_val(s4.sampleCnt, SAMPLE_WAIT_CNT_ABS); 	\
							s4.img.zOffset = dac_get_val(DAC_ZOFFSET_FINE);			 					\
							adc_start_conv(ADC_PHASE);								 					\
							s4.img.zPhase = adc_get_avgw_val(s4.sampleCnt, SAMPLE_WAIT_CNT_ABS);

#define SEND_DATA_TO_CLIENT uart_set_char((s4.img.zAmplitude & 0xFF)); 									\
							uart_set_char((s4.img.zAmplitude >> 8) & 0x0F); 							\
							uart_set_char(s4.img.zOffset & 0xFF); 										\
							uart_set_char((s4.img.zOffset >> 8) & 0x0F); 								\
							uart_set_char(s4.img.zPhase & 0xFF); 										\
							uart_set_char((s4.img.zPhase >> 8) & 0x0F);				

#define SCAN				if (s4.isXScanDirDwn)														\
							{		 																	\
								SCAN_PAIRY_UP;															\
								SCAN_PAIRX_DWN;															\
							}	 																		\
							else																		\
							{																			\
								SCAN_PAIRY_DWN;															\
								SCAN_PAIRX_UP;															\
							}
							
#define SCAN_LAST			SET_PAIRX(s4.xp.a1_val, s4.xp.a2_val);										\
		 					SET_PAIRY(s4.yp.a1_val, s4.yp.a2_val)							

#define SWAP_DIRECTION		if (s4.isXScanDirDwn)														\
								s4.isXScanDirDwn = false;												\
							else				  														\
								s4.isXScanDirDwn = true;		

u16 f_index = (4095*2);

void s4_get_array_flash(void)
{
	GET_DAC_VAL(63);
//	f_index -= 2;
	uart_set_char(s4.data);
	uart_set_char(s4.data >> 8);	
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
}

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

bool s4_scan = false;
				  
void scan4_start (void)
{
//	scan4_init();
	s4_scan = true;
	SET_PAIRX(4095, 0);
	SET_PAIRY((s4.yRange-1), (s4.xRange-1));
	s4.isXScanDirDwn = true;
}

void scan4_step (void)
{
	u8 i;
	bool scan_last_p = false;

	// this loop keeping track of the number of lines that need to be scanned
	// this is essentially the whole scan proccess
	for ( ; s4.lineCnt < s4.numLines; s4.lineCnt++)
	{
		// scan one line back and forth
		for ( ; s4.j < 2; s4.j++)
		{
			for ( ; s4.xStepCnt < s4.numPts; )
			{
				// get 8 data points
				// send data to client
				for (i = 0; i < 8; i++)
				{
					if (!s4_scan)
					{
						if (!scan_last_p)
						{
							SCAN;
						}
						else
						{
							SCAN_LAST;
							scan_last_p = false;
						}
					}
					else
					{
						s4_scan = false;
					}

					// dwell
					//DELAY_MS(s4.dwellTime_ms);
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
			scan_last_p = true;
			SWAP_DIRECTION;
		}
		s4.j = 0;
		SCAN_NEXT_LINE;
	}
	// reset for the next scan
	s4.lineCnt = 0;
}

void scan4_init (void)
{
	s4.f.adr  		= BLOCK0_BASE;
	s4.xp.a1  		= DAC_X2;
	s4.xp.a2  		= DAC_X1;
	s4.yp.a1  		= DAC_Y1;
	s4.yp.a2  		= DAC_Y2;
	s4.xStepCnt 	= 0;
	s4.lineCnt 		= 0;
	s4.dwellTime_ms = 1;
	s4.sampleCnt 	= 2;
}

void s4_set_dwell_t_ms (u8 dwell_ms)
{
	s4.dwellTime_ms = dwell_ms;	
}

void s4_set_sample_cnt (u8 sample_cnt)
{
	s4.sampleCnt = sample_cnt;
}
