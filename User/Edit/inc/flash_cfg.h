#ifndef __FLASH_CFG_H
#define __FLASH_CFG_H

#include <stdint.h>

// Non-volatile user configuration stored in the last sector of internal flash.
// Survives full power loss (unlike RTC backup registers, which need VBAT).
//
// Storage strategy: append-until-full log. Each save writes a new 32-byte flash
// word; the sector is erased only when it fills up (once every ~4096 saves).

void    v_Flash_Cfg_Save_VolLevel(uint8_t u8_lv);
uint8_t u8_Flash_Cfg_Load_VolLevel(void);

#endif
