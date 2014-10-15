#ifndef __DDS_H
#define __DDS_H

#include "../global/global.h"

void dds_spi_init(void);
void dds_write(void);
void dds_step(void);
void dds_get_data(void);
void dds_zoom(void);

void dds_handler(void);
void dds_get_all_data(void);
u32 dds_get_freq_abs(void);
u32 dds_get_freq_hz(void);

void dds_inc(void);
void dds_dec(void);

/* TODO: REMOVE
u8 dds_data[14] = {
	// Write ctrl register 	
    0x0F,
	0xFF,
	
	// Write start frequency
	0xC2,
	0xFF,
	0xD0,
	0x00, 
	
	// Write frequency increment
	0x20,
	0xFF,
	0x30,
	0x00,
	
	// Write number of increments
	0x10,
	0x0F,
	
	// Write increment interval
	0x50,
	0x02
};
*/

#endif
