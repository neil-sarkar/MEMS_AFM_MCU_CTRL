#ifndef __I2C_H
#define __I2C_H

#include "../global/global.h"

void i2c_init (void);

void i2c_handler (void);

void i2c_write_char(u8 ch);
void i2c_send_read(void);


u32 i2c_write_bytes (u8* ptr, u32 size);

#endif
