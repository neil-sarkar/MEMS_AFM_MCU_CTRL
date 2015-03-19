#ifndef __PGA_H
#define __PGA_H

#include "../global/global.h"
#include "../peripheral/uart.h"

void pga_init(void);
void send_address(void);
void pga_get_data (void);
void pga_send(u8);
void mute(u8);

#endif
