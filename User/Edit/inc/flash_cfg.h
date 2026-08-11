#ifndef __FLASH_CFG_H
#define __FLASH_CFG_H

#include <stdint.h>

#define FLASH_CFG_DEVID_LEN		6

// Non-volatile user configuration stored in the last sector of internal flash.
// Survives full power loss, unlike the RTC backup registers — this board has no
// coin cell, and the firmware uses that loss to detect a cold boot.
//
// Storage strategy: append-until-full log. Each save writes a new 32-byte flash
// word holding a full snapshot of every field, so the newest valid entry is
// always self-sufficient. The sector is erased only when it runs low.

// Loads the stored snapshot, increments bootId, and persists it.
//
// MUST run after SystemClock_Config() and before MX_IWDG1_Init(). It may erase
// the sector, which stalls execution for 1-4 s on this single-bank part — longer
// than the 2 s watchdog, and a refresh cannot be interleaved because the CPU
// cannot fetch instructions from flash while it is erasing. Running before the
// watchdog starts removes the conflict instead of working around it.
void     v_Flash_Cfg_Init(void);

// Identifies one contiguous tickMs span, not a power cycle: it advances on every
// reset, which includes the reset v_Mode_WakeUp() issues on power-on. Used as the
// middle element of the SD log merge key (deviceId, bootId, seq).
uint32_t u32_Flash_Cfg_Get_BootId(void);

// FLASH_CFG_DEVID_LEN bytes, all 0xFF until initLogIdentity(0x23) supplies them.
const uint8_t* pu8_Flash_Cfg_Get_DeviceId(void);
void     v_Flash_Cfg_Set_DeviceId(const uint8_t* pu8_id);

void    v_Flash_Cfg_Save_VolLevel(uint8_t u8_lv);
uint8_t u8_Flash_Cfg_Load_VolLevel(void);

#endif
