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
			if ( !flash[which_sector].erased )
			{
				FLASH_Unlock();
				FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
						FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);
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
					if ( FLASH_ProgramByte(address,*buffer++) != FLASH_COMPLETE )
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

/*
void linearFlashProgramStart(struct LinearFlashing* flash) {
  flash->pageBufferTainted = FALSE;
  flash->currentPage = 0;
}

int linearFlashProgramFinish(struct LinearFlashing* flash) {
  int err = 0;

  // Write back last buffer if it is tainted
  if (flash->pageBufferTainted) {
    err = flashPageWriteIfNeeded(flash->currentPage, flash->pageBuffer);
  }

  return err;
}

int linearFlashProgram(struct LinearFlashing* flash, uint32_t address,
                          const flashdata_t* buffer, int length) {
  flashpage_t oldPage;
  int pagePosition;
  int processLen;
  bool_t writeback = FALSE;
  int err;

  // Process all given words
  while(length > 0) {
    oldPage = flash->currentPage;
    flash->currentPage = FLASH_PAGE_OF_ADDRESS(address);
    pagePosition = address - FLASH_ADDRESS_OF_PAGE(flash->currentPage);
    processLen = (FLASH_PAGE_SIZE - pagePosition);

    // Read back new page if page as changed.
    if(oldPage != flash->currentPage) {
      err = flashPageRead(flash->currentPage, flash->pageBuffer);

      // Return if we get errors here.
      if (err == CH_FAILED)
        return -1;

      flash->pageBufferTainted = FALSE;
    }

    // Process no more bytes than remaining
    if(processLen > length) {
      processLen = length;
    } else if (processLen <= length) {
      writeback = TRUE;
    }

    // Copu buffer into page buffer and mark as tainted
    memcpy(&flash->pageBuffer[pagePosition / sizeof(flashdata_t)], buffer,
            processLen);
    flash->pageBufferTainted = TRUE;

    // Decrease handled bytes from total length.
    length -= processLen;

    // Writeback buffer if needed
    if (writeback) {
      err = flashPageWriteIfNeeded(flash->currentPage, flash->pageBuffer);

      // Return if we get errors here.
      if (err)
        return err;

      writeback = FALSE;
    }
  }

  return 0;
}


*/
