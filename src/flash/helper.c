#include <string.h>
#include <ch.h>
#include <hal.h>
#include "helper.h"
#include "flashconfig.h"
#include "stm32f4xx_flash.h"

//-----------------------------------------------------------------------------
void flash_init(struct FlashSector * flash)
{
	uint32_t protected_sectors = FLASH_GetSectorNumber(FLASH_USER_BASE);
	for ( uint32_t i = 0 ; i < FLASH_SECTOR_COUNT ; i++ )
	{
		flash[i].erased = FALSE;
		flash[i].protected = (i < protected_sectors)?TRUE:FALSE;
	}
	FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
			FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);
}

//-----------------------------------------------------------------------------
void flash_finish(struct FlashSector* flash)
{
	(void)flash;
}

//-----------------------------------------------------------------------------
int flash_write(struct FlashSector* flash, uint32_t address, const uint8_t * buffer, int count)
{
	int ret = 0; //  0 == good
	uint32_t which_sector = FLASH_GetSectorNumber( address );
	uint32_t sector_bitmask = FLASH_GetSector( address );

	// if it's out of range, return error
	if ( which_sector < FLASH_SECTOR_COUNT )
	{
		if ( !flash[which_sector].protected )
		{
			FLASH_Unlock();
			FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
					FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);
			if ( !flash[which_sector].erased )
			{
				if ( FLASH_EraseSector(sector_bitmask, VoltageRange_3) == FLASH_COMPLETE )
				{
					FLASH_WaitForLastOperation();
					flash[which_sector].erased = TRUE;
				}
				else
				{
					ret = -3;
					FLASH_Lock();
				}

			}

			if ( flash[which_sector].erased )
			{
				while ( count-- > 0 )
				{

					if ( FLASH_ProgramByte(address++,*buffer++) != FLASH_COMPLETE )
					{
						ret = -4;
						break;
					}
				}

				FLASH_WaitForLastOperation();
				FLASH_Lock();
			}
		}
		else
		{
			ret = -2;
		}
	}
	else
	{
		ret = -1;
	}


	return ret;
}

