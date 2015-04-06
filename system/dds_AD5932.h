#ifndef __DDS_H
#define __DDS_H

#include "../global/global.h"

#ifdef configSYS_DDS_AD5932

void dds_spi_init(void);
void dds_write(void);
void dds_increment(void);
void dds_get_data(void);

void dds_handler(void);
#endif

#endif
