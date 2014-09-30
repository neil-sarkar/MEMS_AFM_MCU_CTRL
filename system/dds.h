#ifndef __DDS_H
#define __DDS_H

#include "../global/global.h"

void dds_spi_init(void);
void dds_write(void);
void dds_increment(void);
void dds_get_data(void);
void dds_zoom(void);

void dds_handler(void);

#endif
