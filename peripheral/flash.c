#include "flash.h"

/*
 *	Initialize Flash Programming Functions
 *	  Parameter:	  adr:	Device Base Address
 *					  clk:	Clock Frequency (Hz)
 *					  fnc:	Function Code (1 - Erase, 2 - Program, 3 - Verify)
 *	  Return Value:   0 - OK,  1 - Failed
 */

u8 flash_Init (void)
{
	FEE0MOD = 0x09;
	FEE1MOD = 0x09;
	return(FEE1HID);
}

/*
 *	De-Initialize Flash Programming Functions
 *	  Parameter:	  fnc:	Function Code (1 - Erase, 2 - Program, 3 - Verify)
 *	  Return Value:   0 - OK,  1 - Failed
 */

u8 flash_UnInit ()
{
	FEE0MOD = 0x00;
	FEE1MOD = 0x00;
	return(0);
}


/*
 *	Erase complete Flash Memory. Do not use. 
 *	  Return Value:   0 - OK,  1 - Failed
 */

u8 flash_EraseChip (void)
{
	#if TRUE
	return 0;
	#else
	unsigned long Flash_Status;
	
	// User Mass Erase of both Blocks
	FEE0MOD = 0x08;
	FEE0ADR = 0xFFC3;
	FEE0DAT = 0x3CFF;
	FEE0CON = MASS_ERASE;
	Flash_Status = FEE0STA;				 //  Load Status of Flash
	while ((Flash_Status & 4) == 4)		 //  Wait until Flash Command
		Flash_Status = FEE0STA; 		 //  is completed
										
	if ((Flash_Status & 2) == 2)		 //  Fail if Fail Bit set
		return(1);						 //  Command Failed
	else
	{
		FEE1MOD  = 0x08;
		FEE1ADR  = 0xFFC3;
		FEE1DAT  = 0x3CFF;
		FEE1CON  = MASS_ERASE;
		Flash_Status = FEE1STA; 		 //  Load Status of Flash
		
		while ((Flash_Status & 4) == 4)	 //  Wait until Flash Command
			Flash_Status = FEE1STA;		 //  is completed
									   
		if ((Flash_Status & 2) == 2)	 //  Fail if Fail Bit set
			return(1);					 //  Command Failed
		else
			return(0);					 //  Command passed
	}
	#endif
}


/*
 *	Check if address is in block 0
 *	  Parameter:	  adr:	Sector Address
 *	  Return Value:   0 - No,  1 - Yes
 *
 *	The ADuc712x can only be configured for 128K
 *	or 96K so both blocks will be present always.
 *
 *	In all ADI sillicon, block 0 is always the highest
 *	block. => >= 64K implies block 0
 */

u8 flash_IsBlk0Addr (u32 adr, u32 size)
{
	if ((adr+size) < (BLOCK0_END) && adr >= BLOCK0_BASE){
		return 1;			
	}
	return 0;
}

u8 flash_IsBlk1Addr (u32 adr, u32 size)
{
	if ((adr+size) < (BLOCK1_END) && adr >= BLOCK1_BASE){
		return 1;			
	}
	return 0;
}

/*
 *	Erase Sector in Flash Memory
 *	  Parameter:	  adr:	Sector Address
 *	  Return Value:   0 - OK,  1 - Failed
 */

u8 flash_EraseSector (u32 adr)
{
	u16 result = 1;
	
	if (flash_IsBlk0Addr(adr, 0))
		result = flash_EraseSectorBlk0(adr & BLOCK_MASK);
	else if (flash_IsBlk1Addr (adr, 0))
		result = flash_EraseSectorBlk1(adr & BLOCK_MASK);
	return result;
}

static u8 flash_EraseSectorBlk0(u16 adr)
{
	unsigned long Flash_Status;
	
	// Start Erase Sector Command
	FEE0ADR	   = adr;					 //  Load Address to erase
	FEE0CON	   = ERASE_PAGE;					 //  Erase Page Command
	Flash_Status = FEE0STA;				 //  Load Status of Flash
	
	while ((Flash_Status & 4) == 4)		 //  Wait until Flash Command
		Flash_Status = FEE0STA; 		 //  is completed
	
	if ((Flash_Status & 2) == 2)		 //  Fail if Fail Bit set
		return(1);						 //  Command Failed
	else								
		return(0);						 //  Command Passed
}

static u8 flash_EraseSectorBlk1(u16 adr)
{
	u32 Flash_Status;
	
	// Start Erase Sector Command
	FEE1ADR	   = adr;					 //  Load Address to erase
	FEE1CON	   = ERASE_PAGE;					 //  Erase Page Command
	Flash_Status = FEE1STA;				 //  Load Status of Flash
	
	while ((Flash_Status & 4) == 4)		 //  Wait until Flash Command
		Flash_Status = FEE1STA;			 //  is completed
	
	if ((Flash_Status & 2) == 2)		 //  Fail if Fail Bit set
		return(1);						 //  Command Failed
	else
		return(0);						 //  Command Passed
}


/*
 *	Program Page in Flash Memory. The location must be erased before a write can occur successfully. 
 *	  Parameter:	  adr:	Page Start Address
 *					  sz:	Page Size
 *					  buf:	Page Data
 *	  Return Value:   0 - OK,  1 - Failed
 */
u8 flash_WriteAdr (u32 adr, u32 sz, u8 *buf)
{
	u8 result = 1;
	
	if (flash_IsBlk0Addr(adr, sz))
		result = flash_ProgramPageBlk0(adr & BLOCK_MASK, sz, buf);
	else if (flash_IsBlk1Addr (adr, sz))
		result = flash_ProgramPageBlk1(adr & BLOCK_MASK, sz, buf);
	
	return result;
}

static u8 flash_ProgramPageBlk0 (u16 adr, u32 sz, u8 *buf)
{
	u16  i;
	u32 Flash_Status;
	
	for (i = 0; i < ((sz+1)/2); i++)
	{ 									 	 //  Write (sz+1)/2 times
		// Start Program Command				 We write in half words
		FEE0ADR = adr + i * 2;				 //  Set Address to write too
		FEE0DAT = *((u16 *) buf); 			 //  Load Data to write
		FEE0CON = WRITE_HALF_WORD; 			 //  Execute Write
		buf += 2;							 //  Increment Buffer location by 2
		Flash_Status = FEE0STA;
		while ((Flash_Status & 4) == 4) 	 //  Wait until Flash command is
		{									 //  completed
			Flash_Status = FEE0STA;
		}
		if ((Flash_Status & 2) == 2)		 //  Fail if Fail Bit set
		{
			return(1);						 //  Command Failed
		}
	}
	return(0);							 	 //  Command Passed
}

static u8 flash_ProgramPageBlk1 (u16 adr, u32 sz, u8 *buf)
{
	u16  i;
	u32 Flash_Status;

  	for (i = 0; i < ((sz+1)/2); i++)
	{ 										 //  Write (sz+1)/2 times
		// Start Program Command				 We write in half words
		FEE1ADR = adr + i * 2;				 //  Set Address to write too

		FEE1DAT = *((u16 *) buf); //  Load Data to write
		FEE1CON = WRITE_HALF_WORD;			 //  Execute Write
		buf += 2;							 //  Increment Buffer location by 2
		Flash_Status = FEE1STA;
		while ((Flash_Status & 4) == 4) 	 //  Wait until Flash command is
		{									 //  completed
			Flash_Status = FEE1STA;
		}
		if ((Flash_Status & 2) == 2)		 //  Fail if Fail Bit set
		{
			return(1);						 //  Command Failed
		}
	}
	return(0);								 //  Command Passed
}


/*
 *	Read Page in Flash Memory
 *	  Parameter:	  adr:	Page Start Address
 *					  sz:	Page Size
 *					  buf:	Page Data
 *	  Return Value:   0 - OK,  1 - Failed
 */
u8 flash_ReadAdr (u32 adr, u32 sz, u8 *buf)
{
	u8 result = 1;
	
	if (flash_IsBlk0Addr(adr, sz))
		result = flash_ReadPageBlk0(adr & BLOCK_MASK, sz, buf);
	else if (flash_IsBlk1Addr(adr, sz))
		result = flash_ReadPageBlk1(adr & BLOCK_MASK,sz,buf);
	
	return result;
}

static u8 flash_ReadPageBlk0 (u16 adr, u32 sz, u8 *buf)
{
	u32 i;
	u32 Flash_Status;
	
	for (i = 0; i < (sz+1)/2; i ++)
	{
		FEE0ADR = adr + i * 2;	    
		FEE0CON = READ_HALF_WORD;
		
		Flash_Status = FEE0STA;
		while ((Flash_Status & 0x04))
		{
			Flash_Status = FEE0STA;
		}
		
		if (Flash_Status & 0x02)
		{
			return 1;
		}
		
		buf[0] = FEE0DAT & 0xFF;
		buf[1] = (FEE0DAT >> 8);
		buf += 2;
	}
	return 0;
}

static u8 flash_ReadPageBlk1 (u16 adr, u32 sz, u8 *buf)
{
	u32 i;
	u32 Flash_Status;
	
	for (i = 0; i < (sz+1)/2; i ++)
	{
		FEE1ADR = adr + i * 2;
		FEE1CON = READ_HALF_WORD;
		
		Flash_Status = FEE1STA;
		while ((Flash_Status & 0x04))
		{
			Flash_Status = FEE1STA;
		}
		
		if (Flash_Status & 0x02)
		{
			return (1);
		}
		
		buf[0] = FEE1DAT & 0xFF;
		buf[1] = (FEE1DAT >> 8);
		buf += 2;
	}
	return (0);
}

u8 flash_Read2Bytes (u32 adr, u16 *data)
{
	u8 result = 1;

	if (flash_IsBlk0Addr(adr, 2))
		result = flash_Read2BytesBlk0 (adr, data);
	else if (flash_IsBlk1Addr(adr, 2))
		result = flash_Read2BytesBlk1 (adr, data);
		
	return result;
}

static u8 flash_Read2BytesBlk0 (u16 adr, u16 *data)
{
	FEE0ADR = adr;
	FEE0CON = READ_HALF_WORD;

	while ((FEE0STA & 0x04)) {}
	
	if (FEE0STA & 0x02) return 1;

	*data = FEE0DAT;
	return 0;
}

static u8 flash_Read2BytesBlk1 (u16 adr, u16 *data)
{
	FEE1ADR = adr;
	FEE1CON = READ_HALF_WORD;

	while ((FEE1STA & 0x04)) {}

	if (FEE1STA & 0x02) return 1;

	*data = FEE1DAT;
	return 0;
}
