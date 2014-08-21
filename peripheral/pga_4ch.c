#include "pga_4ch.h"
#include "../system/wire3.h"

#define PGA_CHNL1_ADR (0x00U)

#define PGA_4CH_DAT_REG		(GP3DAT)
#define PGA_4CH_CS	 		(BIT19)
#define PGA_4CH_CS_DD		(PGA_4CH_CS<<8U)

#define PGA_CHNL_X1			(4-1)
#define PGA_CHNL_X2			(2-1)
#define PGA_CHNL_Y1			(1-1)
#define PGA_CHNL_Y2			(3-1)

struct channel_db {
	u8 x1;
	u8 x2;
	u8 y1;
	u8 y2;
} channel_db;

void pga_4ch_init (void)
{
	PGA_4CH_DAT_REG |= PGA_4CH_CS_DD;
	PGA_4CH_DAT_REG |= PGA_4CH_CS;

	channel_db.x1 = 192;
	channel_db.x2 = 192;
	channel_db.y1 = 192;
	channel_db.y2 = 192;

	wire3_init ();	
}

void pga_4ch_set (pga channel, u8 db)
{
	bool cpol=false;
	u32 data = 0;
	u32 cs_bit = PGA_4CH_CS;
	volatile unsigned long * cs_dat_reg = &PGA_4CH_DAT_REG;

	switch (channel)
	{
		case (pga_x1):
			channel_db.x1 = db;
			break;
		case (pga_x2):
			channel_db.x2 = db;
			break;		
		case (pga_y1):
			channel_db.y1 = db;
			break;		
		case (pga_y2):
			channel_db.y2 = db;
			break;		
	}

	data = (channel_db.x1<<(8*PGA_CHNL_X1)) 
			+ (channel_db.x2<<(8*PGA_CHNL_X2))
			+ (channel_db.y1<<(8*PGA_CHNL_Y1))
			+ (channel_db.y2<<(8*PGA_CHNL_Y2));
	
	wire3_set_cpol (cpol);
	wire3_set_cs (cs_dat_reg, cs_bit);
	wire3_write_wait (data, 4);
}

