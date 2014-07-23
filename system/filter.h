#ifndef __FILTER_H
#define __FILTER_H

#include "../global/global.h"

void filter_adc(void);
void filter_enable(bool enable);
void filter_handler(void);

#endif
