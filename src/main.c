/*
	kuroBox / naniBox
	Copyright (c) 2013
	david morris-oliveros // naniBox.com

    This file is part of kuroBox / naniBox.

    kuroBox / naniBox is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    kuroBox / naniBox is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <ch.h>
#include <hal.h>
#include <chprintf.h>
#include "ff.h"
#include "flashconfig.h"
#include "flash/ihex.h"
#include "flash/helper.h"

//-----------------------------------------------------------------------------
#define CONNECT_TIMEOUT_MS      		500
#define FIRMWARE_FILENAME       		"KUROBOX.HEX"

//-----------------------------------------------------------------------------
#define BOOTLOADER_ERROR_NOCARD     	1
#define BOOTLOADER_ERROR_BADFS      	2
#define BOOTLOADER_ERROR_NOFILE     	3
#define BOOTLOADER_ERROR_BAD_HEX        4
#define BOOTLOADER_ERROR_BAD_FLASH      5
#define BOOTLOADER_ERROR_OTHER          6

//-----------------------------------------------------------------------------
static void loaderError(unsigned int errno);
static void jumpToApp(uint32_t address);
void panic_panic(void);

//-----------------------------------------------------------------------------
static const SDCConfig sdio_cfg = {
	0
};

//-----------------------------------------------------------------------------
#if HAL_USE_SERIAL
static SerialConfig serial1_cfg = {
	115200,
	0,
	USART_CR2_STOP1_BITS | USART_CR2_LINEN,
	0
};
#endif

//-----------------------------------------------------------------------------
static FATFS SDC_FS;
struct FlashSector Flash[FLASH_SECTOR_COUNT];
static IHexRecord irec;
static uint16_t addressOffset;
static uint32_t address;
static enum IHexErrors ihexError;

//-----------------------------------------------------------------------------
int main(void)
{
	halInit();
	chSysInit();
	chThdSleepMilliseconds(1);

	palSetPad(GPIOB, GPIOB_LED1);
	palSetPad(GPIOB, GPIOB_LED2);
	palSetPad(GPIOA, GPIOA_LED3);

	// make them press the damm buttons for 2 seconds...
	for ( int i = 0 ; i < 2000 ; i++ )
	{
		chThdSleepMilliseconds(1);
		if ( (palReadPad(GPIOA, GPIOA_BTN0)==1) || (palReadPad(GPIOA, GPIOA_BTN1)==1) ) // HIGH == not pressed
		{
			// buttons aren't set, don't bootload!
			jumpToApp(FLASH_USER_BASE);
			return 0;
		}
	}

	for ( int i = 0 ; i < 20 ; i++ )
	{
		palTogglePad(GPIOB, GPIOB_LED1);
		palTogglePad(GPIOB, GPIOB_LED2);
		palTogglePad(GPIOA, GPIOA_LED3);
		chThdSleepMilliseconds(50);
	}

#if HAL_USE_SERIAL
	sdStart(&SD1, &serial1_cfg);
	BaseSequentialStream * prnt = (BaseSequentialStream *)&SD1;
	chprintf(prnt, "%s (bootloader)\n\r\n\r", BOARD_NAME);
#endif

	chThdSleepMilliseconds(500);
	palClearPad(GPIOB, GPIOB_LED1);
	palClearPad(GPIOB, GPIOB_LED2);
	palClearPad(GPIOA, GPIOA_LED3);
	chThdSleepMilliseconds(500);

	sdcStart(&SDCD1, &sdio_cfg);

	if (!sdc_lld_is_card_inserted(&SDCD1))
		loaderError(BOOTLOADER_ERROR_NOCARD);
	if (sdcConnect(&SDCD1))
		loaderError(BOOTLOADER_ERROR_NOCARD);

	chThdSleepMilliseconds(150);
	int err = f_mount(0, &SDC_FS);
	if (err != FR_OK)
	{
		sdcDisconnect(&SDCD1);
		loaderError(BOOTLOADER_ERROR_BADFS);
	}

	chThdSleepMilliseconds(150);
	FIL fp;
	if (f_open(&fp, FIRMWARE_FILENAME, FA_READ) != FR_OK)
		loaderError(BOOTLOADER_ERROR_NOFILE);

	// we start flashing now.
	flash_init(Flash);
	int records = 0;

	while ((ihexError = Read_IHexRecord(&irec, &fp)) == IHEX_OK)
	{
		if ( records++%150==0 )
			palTogglePad(GPIOB, GPIOB_LED1);
		switch (irec.type)
		{
		case IHEX_TYPE_00: //< Data Record
			address = (((uint32_t) addressOffset) << 16) + irec.address;

#if HAL_USE_SERIAL
			chprintf(prnt, "Address: %.8x : %.8x\n\r", address, irec.address);
#endif
			if (flash_write(Flash, address, irec.data, irec.dataLen) != 0)
				loaderError(BOOTLOADER_ERROR_BAD_FLASH);

			break;

		case IHEX_TYPE_04: //< Extended Linear Address Record
			addressOffset = (((uint16_t) irec.data[0]) << 8) + irec.data[1];
			break;

		case IHEX_TYPE_01: //< End of File Record
		case IHEX_TYPE_05: //< Start Linear Address Record
			break;

		case IHEX_TYPE_02: //< Extended Segment Address Record
		case IHEX_TYPE_03: //< Start Segment Address Record
			loaderError(BOOTLOADER_ERROR_BAD_HEX);
			break;
		}
	}

	palClearPad(GPIOB, GPIOB_LED1);

	f_close(&fp);

	// Remove firmware so that we do not reflash if something goes wrong
	f_unlink(FIRMWARE_FILENAME);

	// unmounts it
	f_mount(0, NULL);

	sdcConnect(&SDCD1);

	if (err)
		loaderError(BOOTLOADER_ERROR_BAD_FLASH);

	switch (ihexError)
	{
	case IHEX_OK:
	case IHEX_ERROR_EOF:
		break;
	case IHEX_ERROR_FILE:
	case IHEX_ERROR_INVALID_RECORD:
	case IHEX_ERROR_INVALID_ARGUMENTS:
	case IHEX_ERROR_NEWLINE:
		loaderError(BOOTLOADER_ERROR_BAD_HEX);
		break;
	}

	// all good, give a thumbs up and jump to app
	palSetPad(GPIOB, GPIOB_LED1);
	palSetPad(GPIOB, GPIOB_LED2);
	chThdSleepMilliseconds(1000);
	palClearPad(GPIOB, GPIOB_LED1);
	palClearPad(GPIOB, GPIOB_LED2);

	jumpToApp(FLASH_USER_BASE);
	return 0;
}


//-----------------------------------------------------------------------------
// Code stolen from "matis"
// http://forum.chibios.org/phpbb/viewtopic.php?f=2&t=338
static void jumpToApp(uint32_t address)
{
	typedef void (*pFunction)(void);

	pFunction Jump_To_Application;

	// variable that will be loaded with the start address of the application
	vu32* JumpAddress;
	const vu32* ApplicationAddress = (vu32*) address;

	// get jump address from application vector table
	JumpAddress = (vu32*) ApplicationAddress[1];

	// load this address into function pointer
	Jump_To_Application = (pFunction) JumpAddress;

	// reset all interrupts to default
	chSysDisable();

	// Clear pending interrupts just to be on the save side
	SCB_ICSR = ICSR_PENDSVCLR;

	// Disable all interrupts
	int i;
	for (i = 0; i < 8; i++)
		NVIC->ICER[i] = NVIC->IABR[i];

	// set stack pointer as in application's vector table
	__set_MSP((u32)(ApplicationAddress[0]));
	Jump_To_Application();
}

//-----------------------------------------------------------------------------
static void loaderError(unsigned int errno)
{
	unsigned int i;
	palClearPad(GPIOB, GPIOB_LED1);
	palClearPad(GPIOB, GPIOB_LED2);
	palClearPad(GPIOA, GPIOA_LED3);

	for (i = 0; i < errno*2; i++)
	{
		palTogglePad(GPIOA, GPIOA_LED3);
		chThdSleepMilliseconds(500);
	}

	palClearPad(GPIOA, GPIOA_LED3);
	chThdSleepMilliseconds(1500);

	jumpToApp(FLASH_USER_BASE);
}

//-----------------------------------------------------------------------------
void panic_panic(void)
{
	loaderError(BOOTLOADER_ERROR_OTHER);
}

//-----------------------------------------------------------------------------
void assert_param(int statement)
{
	if ( !statement )
		panic_panic();
}
