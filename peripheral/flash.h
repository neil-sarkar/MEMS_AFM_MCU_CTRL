#ifndef __FLASH_H
#define __FLASH_H

#include <flash.h>
#include "../global/global.h"

#define FLASH_MASK  0x1FFFFU
#define BLOCK_MASK  0x0FFFFU
#define PAGE_SIZE 	512
#define _64K        0x10000U

#define BLOCK0_BASE	0x90000U
#define BLOCK0_SIZE	(1024U * 62U)
#define BLOCK0_END	(BLOCK0_BASE + BLOCK0_SIZE) 
#define BLOCK1_BASE	0x80000
#define BLOCK1_SIZE	(1024U * 64U)
#define BLOCK1_END	(BLOCK1_BASE + BLOCK1_SIZE)

u8 flash_Init (void);
u8 flash_UnInit (void);
u8 flash_EraseChip (void);

u8 flash_IsBlk0Addr		(u32 adr, u32 size);
u8 flash_IsBlk1Addr    	(u32 adr, u32 size);

u8 flash_EraseSector 	   (u32 adr);
static u8 flash_EraseSectorBlk0  (u16 adr);
static u8 flash_EraseSectorBlk1  (u16 adr);

u8 flash_WriteAdr      (u32 adr, u32 sz, u8 *buf);
static u8 flash_ProgramPageBlk0  (u16 adr, u32 sz, u8 *buf);
static u8 flash_ProgramPageBlk1  (u16 adr, u32 sz, u8 *buf);

u8 flash_ReadAdr (u32 adr, u32 sz, u8 *buf);
static u8 flash_ReadPageBlk0 (u16 adr, u32 sz, u8 *buf);
static u8 flash_ReadPageBlk1 (u16 adr, u32 sz, u8 *buf);

u8 flash_Read2Bytes (u32 adr, u16 *data);
u8 flash_Read2BytesBlk0 (u16 adr, u16 *data);
u8 flash_Read2BytesBlk1 (u16 adr, u16 *data);

#endif
