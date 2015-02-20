#ifndef __DDSAD9837_H
#define __DDSAD9837_H

#include "../global/global.h"

void dds_AD9837_spi_init(void);
void dds_AD9837_write(void);
void dds_AD9837_increment(u32 pt);
void dds_AD9837_get_data(void);
void dds_AD9837_load_freq(void);
void dds_98_handler(void);

#endif
