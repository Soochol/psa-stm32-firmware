#include "flash_cfg.h"
#include "main.h"
#include "stm32h7xx_hal.h"
#include <string.h>

// STM32H723VG single-bank flash layout:
//   8 sectors × 128 KB = 1 MB total at 0x08000000
//   Sector 7 (last 128 KB, 0x080E0000..0x080FFFFF) reserved here for user cfg.
//
// Current firmware uses sectors 0..6 (~855 KB / 896 KB). As long as code stays
// under 896 KB this sector is free. If size grows past that, reduce LENGTH in
// STM32H723VGTX_FLASH.ld or move this region.
#define FLASH_CFG_SECTOR       FLASH_SECTOR_7
#define FLASH_CFG_BANK         FLASH_BANK_1
#define FLASH_CFG_BASE         0x080E0000U
#define FLASH_CFG_SIZE         0x00020000U                       // 128 KB
#define FLASH_CFG_END          (FLASH_CFG_BASE + FLASH_CFG_SIZE)

// STM32H7 program unit = 256 bits = 32 bytes ("flash word")
#define FLASH_WORD_SIZE        32U
#define FLASH_CFG_MAGIC        0x53565350U   // 'PSVS' little-endian = "PSA SpkVol Save"

typedef struct __attribute__((aligned(32))) {
	uint32_t u32_magic;
	uint8_t  u8_vol;
	uint8_t  u8_vol_inv;   // XOR check (vol ^ vol_inv == 0xFF)
	uint8_t  u8_pad[26];   // 0xFF fill keeps the slot distinguishable from erased state
} x_flash_entry_t;

_Static_assert(sizeof(x_flash_entry_t) == FLASH_WORD_SIZE,
		"flash cfg entry must match flash word size");

static int b_entry_valid(const x_flash_entry_t* p){
	return (p->u32_magic == FLASH_CFG_MAGIC) &&
	       ((uint8_t)(p->u8_vol ^ p->u8_vol_inv) == 0xFFU);
}

static int b_entry_erased(uint32_t u32_addr){
	const uint32_t* p = (const uint32_t*)u32_addr;
	for(uint32_t i = 0; i < FLASH_WORD_SIZE / sizeof(uint32_t); i++){
		if(p[i] != 0xFFFFFFFFU) return 0;
	}
	return 1;
}

static uint32_t u32_find_last_entry(void){
	for(uint32_t addr = FLASH_CFG_END - FLASH_WORD_SIZE;
	    addr >= FLASH_CFG_BASE;
	    addr -= FLASH_WORD_SIZE){
		const x_flash_entry_t* p = (const x_flash_entry_t*)addr;
		if(b_entry_valid(p)) return addr;
		if(addr == FLASH_CFG_BASE) break;   // guard against uint underflow
	}
	return 0U;
}

static uint32_t u32_find_next_free(void){
	for(uint32_t addr = FLASH_CFG_BASE; addr < FLASH_CFG_END; addr += FLASH_WORD_SIZE){
		if(b_entry_erased(addr)) return addr;
	}
	return 0U;
}

static HAL_StatusTypeDef e_erase_sector(void){
	FLASH_EraseInitTypeDef x_erase;
	uint32_t u32_err;
	memset(&x_erase, 0, sizeof(x_erase));
	x_erase.TypeErase    = FLASH_TYPEERASE_SECTORS;
	x_erase.Banks        = FLASH_CFG_BANK;
	x_erase.Sector       = FLASH_CFG_SECTOR;
	x_erase.NbSectors    = 1;
	x_erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
	return HAL_FLASHEx_Erase(&x_erase, &u32_err);
}

void v_Flash_Cfg_Save_VolLevel(uint8_t u8_lv){
	x_flash_entry_t x_entry;
	memset(&x_entry, 0xFF, sizeof(x_entry));
	x_entry.u32_magic  = FLASH_CFG_MAGIC;
	x_entry.u8_vol     = u8_lv;
	x_entry.u8_vol_inv = (uint8_t)(~u8_lv);

	if(HAL_FLASH_Unlock() != HAL_OK) return;

	uint32_t u32_addr = u32_find_next_free();
	if(u32_addr == 0U){
		if(e_erase_sector() != HAL_OK){
			HAL_FLASH_Lock();
			return;
		}
		u32_addr = FLASH_CFG_BASE;
	}

	HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, u32_addr, (uint32_t)&x_entry);
	HAL_FLASH_Lock();
}

uint8_t u8_Flash_Cfg_Load_VolLevel(void){
	uint32_t u32_addr = u32_find_last_entry();
	if(u32_addr == 0U) return 1U;   // cold start / erased sector → Lv1
	const x_flash_entry_t* p = (const x_flash_entry_t*)u32_addr;
	if(p->u8_vol < 1U || p->u8_vol > 3U) return 1U;
	return p->u8_vol;
}
