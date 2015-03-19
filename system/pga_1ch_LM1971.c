#include "pga_1ch_LM1971.h"
#include "../peripheral/wire3.h"

#ifdef configSYS_PGA_LM1971_PGA4311

#define PGA_CHNL1_ADR (0x00U)

#define PGA_FINE_DAT_REG	(GP3DAT)
#define PGA_FINE_CS			(BIT16)
#define PGA_FINE_CS_DD		(PGA_FINE_CS<<8U)

#define PGA_DDS_DAT_REG		(GP0DAT)
#define PGA_DDS_CS			(BIT22)
#define PGA_DDS_CS_DD		(PGA_DDS_CS<<8U)

void pga_1ch_init (pga channel)
{
	switch (channel)
	{
		case (pga_fine):
			PGA_FINE_DAT_REG |= PGA_FINE_CS_DD;
			PGA_FINE_DAT_REG |= PGA_FINE_CS;
			break;
		case (pga_dds):
			PGA_DDS_DAT_REG |= PGA_DDS_CS_DD;
			PGA_DDS_DAT_REG |= PGA_DDS_CS;
			break;
	}
	// Disabled due to being done in 4ch init
	wire3_init ();	
}

void pga_1ch_set (pga channel, u8 db)
{
	bool cpol=true;
	u32 data = 0;
	u32 cs_bit;
	volatile unsigned long * cs_dat_reg;

	switch (channel)
	{
		case (pga_fine):
			cs_dat_reg = &PGA_FINE_DAT_REG;
			cs_bit = PGA_FINE_CS;
			data = (PGA_CHNL1_ADR << 8) |  db;
			break;
		case (pga_dds):
			cs_dat_reg = &PGA_DDS_DAT_REG;
			cs_bit = PGA_DDS_CS;
			data = (PGA_CHNL1_ADR << 8) | db;
			break;		
	}
	
	wire3_set_cpol (cpol);
	wire3_set_cs (cs_dat_reg, cs_bit);
	wire3_write_wait ((u16)data, 2);
}

#endif

