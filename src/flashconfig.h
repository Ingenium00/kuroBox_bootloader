#ifndef FLASHCONFIG_H_
#define FLASHCONFIG_H_

/**
 * @brief Start address of user application, anything below this would be invalid
 */
#if !defined(FLASH_USER_BASE) || defined(__DOXYGEN__)
#define FLASH_USER_BASE         ((uint32_t)0x08020000)
#endif

#define FLASH_SECTOR_COUNT	12

#endif /* FLASHCONFIG_H_ */
